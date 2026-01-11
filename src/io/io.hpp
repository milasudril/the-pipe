#ifndef PROG_IO_HPP
#define PROG_IO_HPP

#include "src/utils/file_descriptor.hpp"
#include "src/utils/system_error.hpp"
#include <cerrno>
#include <expected>

/**
 * \brief Contains basic I/O support functions
 */
namespace prog::io
{
	/**
	 * \brief Tag used to detect that an I/O operation could not complete without blocking
	 */
	struct retry_tag{};

	/**
	 * \brief Tag used to identify a file descriptor that can be read from
	 */
	struct input_file_descriptor_tag{};

	/**
	 * \brief A reference to a file descriptor that can be read from
	 */
	using input_file_descriptor_ref = utils::tagged_file_descriptor_ref<input_file_descriptor_tag>;

	/**
	 * \brief Tries to read data from fd into buffer
	 * \return If the operation would not block, the part of buffer that has not yet been filled,
	 *         otherwise a retry_tag
	 * \throw utils::system_error for any other error
	 */
	std::expected<std::span<std::byte>, retry_tag>
	read(input_file_descriptor_ref fd, std::span<std::byte> buffer)
	{
		auto const res = ::read(fd.native_handle(), std::data(buffer), std::size(buffer));
		if(res != -1) [[likely]]
		{ return std::span{std::begin(buffer) + res, std::end(buffer)}; }

		auto const err = errno;

		if(err == EAGAIN || errno == EWOULDBLOCK)
		{ return std::unexpected(retry_tag{}); }

		throw utils::system_error{"`read` failed", err};
	}

	/**
	 * \brief Tag used to identify a file descriptor that can be written to
	 */
	struct output_file_descriptor_tag
	{};

	/**
	 * \brief Tag used to identify a file descriptor that can be written to
	 */
	using output_file_descriptor_ref = utils::tagged_file_descriptor_ref<output_file_descriptor_tag>;

	/**
	 * \brief Tries to write data from buffer to fd
	 * \return If the operation would not block, the part of buffer that has not yet been written,
	 *         otherwise a retry_tag
	 * \throw utils::system_error for any other error
	 */
	std::expected<std::span<std::byte const>, retry_tag>
	write(output_file_descriptor_ref fd, std::span<std::byte const> buffer)
	{
		auto const res = ::write(fd.native_handle(), std::data(buffer), std::size(buffer));
		if(res != -1) [[likely]]
		{ return std::span{std::begin(buffer) + res, std::end(buffer)}; }

		auto const err = errno;

		if(err == EAGAIN || err == EWOULDBLOCK)
		{ return std::unexpected(retry_tag{}); }

		throw utils::system_error{"`write` failed", err};
	}
}

#endif