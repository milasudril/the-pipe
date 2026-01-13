//@	{"dependencies_extra":[{"ref":"./proc_mgmt.o", "rel":"implementation"}]}

#ifndef PROG_PROC_MGMT_HPP
#define PROG_PROC_MGMT_HPP

#include "src/io/io.hpp"
#include "src/utils/file_descriptor.hpp"
#include "src/utils/system_error.hpp"
#include <csignal>
#include <sys/wait.h>
#include <filesystem>

namespace prog::proc_mgmt
{
	struct pidfd_tag
	{};

	using pidfd_ref = utils::tagged_file_descriptor_ref<pidfd_tag>;
	using pidfd = utils::tagged_file_descriptor<pidfd_tag>;

	struct io_redirection
	{
		io::input_file_descriptor_ref sysin;
		io::output_file_descriptor_ref sysout;
		io::output_file_descriptor_ref syserr;
	};

	pidfd spawn(
		char const* path,
		std::span<char const*> argv,
		std::span<char const*> env,
		io_redirection const& io_redir
	);

	inline void kill(pidfd_ref fd, int signo)
	{
		auto res = syscall(SYS_pidfd_send_signal, fd.native_handle(), signo, nullptr, 0);
		if(res == -1)
		{ throw utils::system_error{"Failed to kill process", errno}; }
	}

	struct process_exited
	{
		int return_value;
	};

	struct process_killed
	{
		int signo;
	};

	using process_termination_status = std::variant<process_exited, process_killed>;

	inline process_termination_status wait(pidfd_ref fd)
	{
		siginfo_t siginfo{};
		auto res = ::waitid(P_PIDFD, fd.native_handle(), &siginfo, WEXITED);
		if(res == -1)
		{ throw utils::system_error{"Failed to wait for process", errno}; }

		if(siginfo.si_code == CLD_EXITED)
		{
			return process_exited{
				.return_value = siginfo.si_status
			};
		}
		else
		{
			return process_killed{
				.signo = siginfo.si_status
			};
		}
	}
}

#endif
