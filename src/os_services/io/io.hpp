#ifndef PIPE_OS_SERVICES_IO_HPP
#define PIPE_OS_SERVICES_IO_HPP

#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/error_handling/system_error.hpp"
#include "src/os_services/error_handling/error_handling.hpp"

#include <cerrno>
#include <expected>
#include <cassert>
#include <type_traits>

/**
 * \brief Contains basic I/O support functions
 */
namespace Pipe::os_services::io
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
		explicit io_result(ssize_t value, int err):
			m_value{value},
			m_operation_would_have_blocked{err == EAGAIN || err == EWOULDBLOCK}
		{
			if(err != 0 && err != EPIPE && !operation_would_have_blocked())
			{ throw error_handling::system_error{"I/O operation failed", err}; }
		}

		/**
		 * \brief Indicates whether or not the I/O operation would have blocked the calling thread
		 */
		[[nodiscard]] bool operation_would_have_blocked() const noexcept
		{ return m_operation_would_have_blocked; }

		/**
		 * \brief Returns the number of bytes transferred during the I/O operation
		 */
		[[nodiscard]] size_t bytes_transferred() const noexcept
		{ return m_value < 0? static_cast<size_t>(0): static_cast<size_t>(m_value); }

	private:
		ssize_t m_value;
		bool m_operation_would_have_blocked;
	};

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
	{ return error_handling::do_while_eintr(::read, fd, buffer, count); }

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
	 * \brief Reads as much as possible into buffer
	 * \return An io_result, containing the number of bytes transferred during the operation
	 */
	inline io_result read_full(input_file_descriptor_ref fd, std::span<std::byte> buffer)
	{
		auto const start_at = std::begin(buffer);
		while(!buffer.empty())
		{
			const auto read_result = read(fd, buffer);
			if(read_result.bytes_transferred() == 0)
			{
				return io_result{
					std::begin(buffer) - start_at,
					read_result.operation_would_have_blocked()? EAGAIN: 0
				};
			}

			buffer = std::span{
				std::begin(buffer) + read_result.bytes_transferred(),
				std::end(buffer)
			};
		}

		return io_result{std::begin(buffer) - start_at, 0};
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
	{ return error_handling::do_while_eintr(::write, fd, buffer, count); }

	/**
	 * \brief Tries to write data from buffer to fd
	 * \return An io_result, containing the number of bytes transferred during the operation
	 */
	inline io_result write(output_file_descriptor_ref fd, std::span<std::byte const> buffer)
	{
		return io_result{
			write_while_eintr(fd.native_handle(), std::data(buffer), std::size(buffer)),
			errno
		};
	}

		/**
	 * \brief Writes as much as to fd
	 * \return An io_result, containing the number of bytes transferred during the operation
	 */
	inline io_result write_full(output_file_descriptor_ref fd, std::span<std::byte const> buffer)
	{
		auto const start_at = std::begin(buffer);
		while(!buffer.empty())
		{
			auto const write_result = write(fd, buffer);
			if(write_result.bytes_transferred() == 0)
			{
				return io_result{
					std::begin(buffer) - start_at,
					write_result.operation_would_have_blocked()? EAGAIN: 0
				};
			}

			buffer = std::span{
				std::begin(buffer) + write_result.bytes_transferred(),
				std::end(buffer)
			};
		}

		return io_result{std::begin(buffer) - start_at, 0};
	}

	struct fd_closed{};
	struct io_blocked{};

	template<class Byte, class OnBlocked, class OnClosed>
	requires(
		std::is_same_v<std::remove_cv_t<Byte>, std::byte> ||
		std::is_same_v<std::remove_cv_t<Byte>, char> ||
		std::is_same_v<std::remove_cv_t<Byte>, char8_t>
	)
	bool try_write(
		output_file_descriptor_ref fd,
		std::span<Byte>& buffer,
		OnBlocked&& on_blocked,
		OnClosed&& on_closed
	)
	{
		if(buffer.empty())
		{ return true; }

		auto const write_result = Pipe::os_services::io::write_full(fd, std::as_bytes(buffer));
		buffer = std::span{
			std::begin(buffer) + write_result.bytes_transferred(),
			std::end(buffer)
		};

		if(write_result.operation_would_have_blocked())
		{
			std::forward<OnBlocked>(on_blocked)(io_blocked{});
			return false;
		}

		if(write_result.bytes_transferred() == 0)
		{
			std::forward<OnClosed>(on_closed)(fd_closed{});
			return false;
		}

		return true;
	}
}

#endif