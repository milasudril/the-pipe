//@	{"dependencies_extra":[{"ref":"./proc_mgmt.o", "rel":"implementation"}]}

#ifndef PROG_PROC_MGMT_HPP
#define PROG_PROC_MGMT_HPP

#include "src/io/io.hpp"
#include "src/utils/file_descriptor.hpp"
#include "src/utils/system_error.hpp"
#include <csignal>
#include <sys/wait.h>
#include <filesystem>

/**
 * \brief Process management
 */
namespace prog::proc_mgmt
{
	/**
	 * \brief Tag type used to identify a file descriptor referring to a process
	 */
	struct pidfd_tag
	{};

	/**
	 * \brief A reference to a process file descriptor
	 */
	using pidfd_ref = utils::tagged_file_descriptor_ref<pidfd_tag>;

	/**
	 * \brief An owner type for a process file descriptor
	 */
	using pidfd = utils::tagged_file_descriptor<pidfd_tag>;

	/**
	 * \brief Provides information about redirection of the standard streams
	 */
	struct io_redirection
	{
		io::input_file_descriptor_ref sysin;   /**<\brief stdin*/
		io::output_file_descriptor_ref sysout; /**<\brief stdout*/
		io::output_file_descriptor_ref syserr; /**<\brief stderr*/
	};

	/**
	 * \brief Spawns a new process
	 *
	 * \param path The path to the executable file. Notice that the PATH variable is not taken into
	 *             account.
	 *
	 * \param argv The argument to be passed to `main`. The application path is prepended
	 *             automatically
	 *
	 * \param env A list of environment variable strings, on the form key=value
	 *
	 * \param io_redir An io_redirection object used to configure redirection of the standard streams.
	 */
	pidfd spawn(
		char const* path,
		std::span<char const*> argv,
		std::span<char const*> env,
		io_redirection const& io_redir
	);

	/**
	 * \brief Kills the process referred to by fd, by signo
   */
	inline void kill(pidfd_ref fd, int signo)
	{
		auto res = syscall(SYS_pidfd_send_signal, fd.native_handle(), signo, nullptr, 0);
		if(res == -1)
		{ throw utils::system_error{"Failed to kill process", errno}; }
	}

	/**
	 * \brief Holder for a process return value
	 */
	struct process_exited
	{
		int return_value;
	};

	/**
	 * \brief Holder for a process kill signal
	 */
	struct process_killed
	{
		int signo;
	};

	/**
	 * \brief Holder to store the result of a wait operation
	 */
	using process_termination_status = std::variant<process_exited, process_killed>;

	/**
	 * \brief Waits for the process referred to by fd to either exit or being killed
	 */
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
