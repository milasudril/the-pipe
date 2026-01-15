//@	{"target":{"name":"proc_mgmt.o"}}

#include "./proc_mgmt.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/eventfd.hpp"
#include "src/os_services/fd/file_descriptor.hpp"

#include <cstdint>
#include <linux/close_range.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <signal.h>
#include <sys/wait.h>

#ifdef COVERAGE_BUILD
extern "C" void __gcov_dump();
#else
#define __gcov_dump()
#endif

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

		close_range(STDERR_FILENO + 1, ~0u, CLOSE_RANGE_CLOEXEC);
		__gcov_dump();
		execve(path, argv, env);

	fail:
		auto errval = errno;
		prog::io::write_while_eintr(errstream, &errval, sizeof(errval));
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
	auto parent_ready_fd = ipc::make_eventfd();

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
			throw error_handling::system_error{"Fork failed: ", errno};

		case 0:
		{
			// In child
			uint64_t val{};
			io::read_while_eintr(parent_ready_fd.get().native_handle(), &val, sizeof(val));
			parent_ready_fd.reset();
			::close(exec_err_pipe_read_end);
			do_exec(path, std::data(argv_out), std::data(env_out), io_redir, exec_err_pipe.write_end());
			exec_err_pipe.close_write_end();
			__gcov_dump();
			_exit(127);
			break;
		}

		default:
		{
			exec_err_pipe.close_write_end();
			// In parent
			auto fd = pidfd_open(fork_res, 0);
			if(fd == -1)
			{
				auto const saved_errno = errno;
				kill(fork_res, SIGKILL);
				waitpid(fork_res, nullptr, 0);
				throw error_handling::system_error{"Failed to create pidfd", saved_errno};
			}
			pidfd ret{fd};
			uint64_t val{1};
			io::write_while_eintr(
				parent_ready_fd.get().native_handle(),
				&val,
				sizeof(val)
			);

			int child_errno{};
			auto const read_result = read(
				exec_err_pipe_read_end,
				std::as_writable_bytes(std::span{&child_errno, 1})
			);
			if(read_result.bytes_transferred() != 0)
			{
				::close(exec_err_pipe_read_end);
				wait(ret.get());
				throw error_handling::system_error{std::format("Failed to launch application {}", path), child_errno};
			}

			::close(exec_err_pipe_read_end);
			return ret;
		}
	};
}