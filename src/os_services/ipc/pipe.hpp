#ifndef PROG_IPC_PIPE_HPP
#define PROG_IPC_PIPE_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <cstdlib>
#include <fcntl.h>

namespace prog::ipc
{
	/**
	 * \brief A pipe is a unidirectional communication channel, with a read end and a write end
	 */
	class pipe
	{
	public:
		/**
		 * \brief Constructs a pipe
		 */
		pipe()
		{
			std::array<int, 2> fds{};
			auto const res = ::pipe(std::data(fds));
			if(res == -1)
			{ throw utils::system_error{"Failed to create pipe", errno}; }

			m_read_end = io::input_file_descriptor{fds[0]};
			m_write_end = io::output_file_descriptor{fds[1]};
		}

		/**
		 * \brief Returns a reference to the read end of the pipe
		 */
		auto read_end() const noexcept
		{ return m_read_end.get(); }

		/**
		 * \brief Returns a reference to the write end of the pipe
		 */
		auto write_end() const noexcept
		{ return m_write_end.get(); }

		/**
		 * \brief Closes the read end of the pipe
		 */
		void close_read_end() noexcept
		{ m_read_end.reset(); }

		/**
		 * \brief Closes the write end of the pipe
		 */
		void close_write_end() noexcept
		{ return m_write_end.reset(); }

		[[nodiscard]] auto close_read_end_on_exec()
		{
			if(fcntl(m_read_end.get().native_handle(), F_SETFD, FD_CLOEXEC) == -1)
			{ throw utils::system_error{"Failed to set FD_CLOEXEC on pipe read end", errno}; }
			return m_read_end.release();
		}

		[[nodiscard]] auto close_write_end_on_exec()
		{
			if(fcntl(m_write_end.get().native_handle(), F_SETFD, FD_CLOEXEC) == -1)
			{ throw utils::system_error{"Failed to set FD_CLOEXEC on pipe write end", errno}; }
			return m_write_end.release();
		}

	private:
		io::input_file_descriptor m_read_end;
		io::output_file_descriptor m_write_end;
	};
}

#endif