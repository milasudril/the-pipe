#ifndef PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP
#define PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP

#include "./file_descriptor.hpp"

#include "src/os_services/error_handling/error_handling.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/epoll.h>

namespace prog::os_services::fd
{
	/**
	 * \brief Tag used to identify a file descriptor for an epoll instance
	 */
	struct epoll_file_descriptor_tag{};

	/**
	 * \brief A reference to a file descriptor that can be used with the epoll API
	 */
	using epoll_file_descriptor_ref = fd::tagged_file_descriptor_ref<epoll_file_descriptor_tag>;

	/**
	 * \brief An owner of a file descriptor that can be used with the epoll API
	 */
	using epoll_file_descriptor = fd::tagged_file_descriptor<epoll_file_descriptor_tag>;

	inline epoll_file_descriptor make_epoll_instance()
	{
		epoll_file_descriptor ret{::epoll_create1(0)};
		if(ret == nullptr)
		{ throw error_handling::system_error{"Failed to create epoll instance", errno}; }
		return ret;
	}
}

#endif