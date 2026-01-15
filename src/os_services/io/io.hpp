#ifndef PROG_IO_HPP
#define PROG_IO_HPP

#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <cerrno>
#include <expected>
#include <cassert>

/**
 * \brief Contains basic I/O support functions
 */
namespace prog::io
{
	/**
	 * \brief A class holding the result of an I/O operation
	 */
	class io_result
	{
	public:
		/**
		 * \brief Constructs an io_result from a ssize_t value, as returned by `::read` or
		 * `::write` syscalls
		 * \note If `value` < 0 and `err` indicates something other than EAGAIN or EWOULDBLOCK
		 * an exception is thrown
		 */
		explicit io_result(ssize_t value, int err): m_value{value}
		{
			if(m_value < 0 && err != EAGAIN && err != EWOULDBLOCK)
			{ throw error_handling::system_error{"I/O operation failed", err}; }
		}

		/**
		 * \brief Indicates whether or not the I/O operation would have blocked the calling thread
		 */
		[[nodiscard]] bool operation_would_have_blocked() const noexcept
		{ return m_value == -1; }

		/**
		 * \brief Returns the number of bytes transferred during the I/O operation
		 */
		[[nodiscard]] size_t bytes_transferred() const noexcept
		{ return m_value < 0? static_cast<size_t>(0): static_cast<size_t>(m_value); }

	private:
		ssize_t m_value;
	};

	/**
	 * \brief Repeatedly calls func, until it returns a value different from -1, or errno is no
	 *        longer EINTR
	 */
	template<class Callable, class... Args>
	auto do_while_eintr(Callable func, Args... args) noexcept
	{
		errno = EINTR;
		std::invoke_result_t<Callable, Args...> result = -1;
		while(errno == EINTR && result == -1)
		{ result = func(args...); }
		return result;
	}

	/**
	 * \brief Tag used to identify a file descriptor that can be read from
	 */
	struct input_file_descriptor_tag{};

	/**
	 * \brief A reference to a file descriptor that can be read from
	 */
	using input_file_descriptor_ref = fd::tagged_file_descriptor_ref<input_file_descriptor_tag>;

	/**
	 * \brief An owner of a file descriptor that can be read from
	 */
	using input_file_descriptor = fd::tagged_file_descriptor<input_file_descriptor_tag>;

	/**
	 * \brief Helper function for writing until EINTR is no longer raised
	 */
	inline auto read_while_eintr(int fd, void* buffer, size_t count) noexcept
	{ return do_while_eintr(::read, fd, buffer, count); }

	/**
	 * \brief Tries to read data from fd into buffer
	 * \return An io_result, containing the number of bytes transferred during the operation
	 */
	inline io_result read(input_file_descriptor_ref fd, std::span<std::byte> buffer)
	{
		return io_result{
			read_while_eintr(fd.native_handle(), std::data(buffer), std::size(buffer)),
			errno
		};
	}

	/**
	 * \brief Tag used to identify a file descriptor that can be written to
	 */
	struct output_file_descriptor_tag
	{};

	/**
	 * \brief Tag used to identify a file descriptor that can be written to
	 */
	using output_file_descriptor_ref = fd::tagged_file_descriptor_ref<output_file_descriptor_tag>;

	/**
	 * \brief An owner of a file descriptor that can be written to
	 */
	using output_file_descriptor = fd::tagged_file_descriptor<output_file_descriptor_tag>;

	/**
	 * \brief Helper function for writing until EINTR is no longer raised
	 */
	inline auto write_while_eintr(int fd, void const* buffer, size_t count) noexcept
	{ return do_while_eintr(::write, fd, buffer, count); }

	/**
	 * \brief Tries to write data from buffer to fd
	 * \return An io_result, containing the number of bytes transferred during the operation
	 */
	inline io_result write(output_file_descriptor_ref fd, std::span<std::byte const> buffer)
	{
		return io_result{
			do_while_eintr(::write, fd.native_handle(), std::data(buffer), std::size(buffer)),
			errno
		};
	}
}

#endif