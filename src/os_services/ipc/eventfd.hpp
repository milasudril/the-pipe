#ifndef PIPE_OS_SERVICES_IPC_EVENTFD_HPP
#define PIPE_OS_SERVICES_IPC_EVENTFD_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <sys/eventfd.h>

namespace Pipe::os_services::ipc
{
	/**
	 * \brief A tag type used to identify an event file descriptor
	 */
	struct eventfd_tag
	{};

	/**
	 * \brief A reference to an event file descriptor
	 */
	using eventfd_ref = fd::tagged_file_descriptor_ref<eventfd_tag>;

	/**
	 * \brief An owner of an event file descriptor
	 */
	using eventfd = fd::tagged_file_descriptor<eventfd_tag>;

	/**
	 * \brief Creates an event file descriptor, to be used for synchronization between processes
	 */
	inline auto make_eventfd()
	{
		eventfd ret{::eventfd(0, 0)};
		if(ret == nullptr)
		{ throw error_handling::system_error{"Failed to create eventfd", errno}; }
		return ret;
	}
}

template<>
struct Pipe::os_services::fd::enabled_fd_conversions<Pipe::os_services::ipc::eventfd_tag>
{
	static consteval void supports(io::input_file_descriptor_tag){}
	static consteval void supports(io::output_file_descriptor_tag){}
};

#endif