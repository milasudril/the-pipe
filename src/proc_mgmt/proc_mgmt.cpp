//@	{"target":{"name":"proc_mgmt.hpp"}}

#include "./proc_mgmt.hpp"
#include "src/io/io.hpp"
#include "src/ipc/pipe.hpp"
#include "src/utils/file_descriptor.hpp"

#include <cstdint>
#include <linux/close_range.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <signal.h>
#include <sys/wait.h>
#include <sys/eventfd.h>

namespace
{
	int pidfd_open(pid_t pid, unsigned int flags) noexcept
	{
		return static_cast<int>(syscall(SYS_pidfd_open, pid, flags));
	}

	void do_exec(
		char const* path,
		char* const* argv,
		char* const* env,
		prog::proc_mgmt::io_redirection const& io_redir,
		prog::io::output_file_descriptor_ref errstream
	) noexcept
	{
		if(io_redir.sysin != nullptr)
		{
			if(dup2(io_redir.sysin, STDIN_FILENO) == -1)
			{ goto fail; }
		}
		if(io_redir.sysout != nullptr)
		{
			if(dup2(io_redir.sysout, STDOUT_FILENO) == -1)
			{ goto fail; }
		}

		if(io_redir.syserr != nullptr)
		{
			if(dup2(io_redir.syserr, STDERR_FILENO) == -1)
			{ goto fail; }
		}

		close_range(STDERR_FILENO + 1, ~0u, CLOSE_RANGE_CLOEXEC | CLOSE_RANGE_UNSHARE);

		execve(path, argv, env);

	fail:
		auto errval = errno;
		write(errstream, std::as_bytes(std::span{&errval, 1}));
	}
};

prog::proc_mgmt::pidfd
prog::proc_mgmt::spawn(
	char const* path,
	std::span<char const*> argv,
	std::span<char const*> env,
	io_redirection const& io_redir
)
{
	ipc::pipe exec_err_pipe;
	auto const exec_err_pipe_read_end = exec_err_pipe.close_read_end_on_exec();

	utils::file_descriptor parent_ready_fd{eventfd(0, 0)};
	if(parent_ready_fd == nullptr)
	{ throw utils::system_error{"Failed to create parent to child sync fd", errno}; }

	// Before fork, prepare stuff to be passed to exec
	std::vector<char*> argv_out{const_cast<char*>(path)};
	for(auto item : argv)
	{ argv_out.push_back(const_cast<char*>(item)); }
	argv_out.push_back(nullptr);

	std::vector<char*> env_out;
	for(auto item : env)
	{ env_out.push_back(const_cast<char*>(item)); }
	env_out.push_back(nullptr);

	auto const fork_res = fork();
	switch(fork_res)
	{
		case -1:
			throw utils::system_error{"Fork failed: ", errno};

		case 0:
		{
			// In child
			uint64_t val{};
			std::ignore = ::read(parent_ready_fd.get().native_handle(), &val, sizeof(val));
			::close(exec_err_pipe_read_end);
			do_exec(path, std::data(argv_out), std::data(env_out), io_redir, exec_err_pipe.write_end());
			exec_err_pipe.close_write_end();
			_exit(127);
			break;
		}

		default:
		{
			// In parent
			auto fd = pidfd_open(fork_res, 0);
			if(fd == -1)
			{
				auto const saved_errno = errno;
				kill(fork_res, SIGKILL);
				waitpid(fork_res, nullptr, 0);
				throw utils::system_error{"Failed to create pidfd", saved_errno};
			}
			uint64_t val{1};
			std::ignore = ::write(parent_ready_fd.get().native_handle(), &val, sizeof(val));

			int child_errno{};
			auto const read_result = read(
				exec_err_pipe_read_end,
				std::as_writable_bytes(std::span{&child_errno, 1})
			);
			if(read_result.bytes_transferred() != 0)
			{ throw utils::system_error{std::format("Failed to launch application {}", path), child_errno}; }


			return pidfd{fd};
		}
	};
}