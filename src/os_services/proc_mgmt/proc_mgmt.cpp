//@	{"target":{"name":"proc_mgmt.o"}}

#include "./proc_mgmt.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/eventfd.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/utils/utils.hpp"

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
		return static_cast<int>(::syscall(SYS_pidfd_open, pid, flags));
	}

	void do_exec(
		char const* path,
		char* const* argv,
		char* const* env,
		Pipe::os_services::proc_mgmt::io_redirection const& io_redir,
		Pipe::utils::immutable_flat_set<unsigned int> fds_to_keep,
		Pipe::os_services::io::output_file_descriptor_ref errstream
	) noexcept
	{
		if(io_redir.sysin != nullptr)
		{
			if(::dup2(io_redir.sysin.get(), STDIN_FILENO) == -1)
			{ goto fail; }
		}
		if(io_redir.sysout != nullptr)
		{
			if(::dup2(io_redir.sysout.get(), STDOUT_FILENO) == -1)
			{ goto fail; }
		}

		if(io_redir.syserr != nullptr)
		{
			if(::dup2(io_redir.syserr.get(), STDERR_FILENO) == -1)
			{ goto fail; }
		}

		for_each_disjoint_segment(
			Pipe::utils::inclusive_integral_range{
				.start_at = static_cast<unsigned int>(STDERR_FILENO + 1),
				.stop_at = ~0u
			},
			fds_to_keep,
			[](auto const range){
				::close_range(range.start_at, range.stop_at, CLOSE_RANGE_CLOEXEC);
			}
		);
		__gcov_dump();
		::execve(path, argv, env);

	fail:
		auto errval = errno;
		Pipe::os_services::io::write_while_eintr(errstream, &errval, sizeof(errval));
	}

	std::vector<char*> build_argv(char const* path, std::span<char const*> argv)
	{
		std::vector<char*> argv_out;
		argv_out.reserve(2 + std::size(argv));
		argv_out.push_back(const_cast<char*>(path));
			for(auto item : argv)
		{ argv_out.push_back(const_cast<char*>(item)); }
		argv_out.push_back(nullptr);
		return argv_out;
	}

	std::vector<char*> build_env(std::span<char const*> env)
	{
		std::vector<char*> env_out;
		env_out.reserve(1 + std::size(env));
		for(auto item : env)
		{ env_out.push_back(const_cast<char*>(item)); }
		env_out.push_back(nullptr);
		return env_out;
	}
};

std::pair<pid_t, Pipe::os_services::proc_mgmt::pidfd>
Pipe::os_services::proc_mgmt::spawn(
	char const* path,
	std::span<char const*> argv,
	std::span<char const*> env,
	io_redirection const& io_redir,
	std::span<fd::file_descriptor> fds_to_forward
)
{
	ipc::pipe exec_err_pipe;
	auto const exec_err_pipe_read_end = exec_err_pipe.close_read_end_on_exec();
	auto parent_ready_fd = ipc::make_eventfd();

	// Before fork, prepare stuff to be passed to do_exec
	auto const argv_out = build_argv(path, argv);
	auto const env_out = build_env(env);
	utils::flat_set fds_to_keep{
		std::begin(fds_to_forward),
		std::end(fds_to_forward),
		[](auto const& val) {
			return static_cast<unsigned int>(val.get().native_handle());
		}
	};

	auto const fork_res = ::fork();
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
			do_exec(
				path,
				std::data(argv_out),
				std::data(env_out),
				io_redir,
				fds_to_keep,
				exec_err_pipe.write_end()
			);
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
			std::pair ret{fork_res, pidfd{fd}};
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
				wait(ret.second.get());
				throw error_handling::system_error{std::format("Failed to launch application {}", path), child_errno};
			}

			::close(exec_err_pipe_read_end);
			return ret;
		}
	};
}