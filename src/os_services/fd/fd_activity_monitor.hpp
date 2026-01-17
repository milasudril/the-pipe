//@	{"dependencies_extra":[{"ref": "./fd_activity_monitor.o", "rel": "implementation"}]}

#ifndef PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP
#define PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP

#include "./file_descriptor.hpp"

#include "src/os_services/error_handling/error_handling.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/epoll.h>
#include <functional>

namespace prog::os_services::fd
{
	enum class activity_status
	{
		read,
		write,
		read_or_write
	};

	constexpr unsigned int to_epoll_event(activity_status status)
	{
		switch(status)
		{
			case activity_status::read:
				return EPOLLIN;
			case activity_status::write:
				return EPOLLOUT;
			case activity_status::read_or_write:
				return EPOLLIN|EPOLLOUT;
		}

		throw std::runtime_error{"Bad activity status"};
	}

	constexpr activity_status epoll_event_to_activity_status(unsigned int event)
	{
		if((event & EPOLLIN) && (event & EPOLLOUT))
		{ return activity_status::read_or_write; }
		if(event & EPOLLIN)
		{ return activity_status::read; }
		if(event & EPOLLOUT)
		{ return activity_status::write; }

		throw std::runtime_error{"Bad epoll event"};
	}

	enum class post_process_action{keep_entry, remove_entry};

	class activity_monitor
	{
	public:
		activity_monitor():
			m_epoll_fd{::epoll_create1(0)}
		{
			if(m_epoll_fd == nullptr)
			{ throw error_handling::system_error{"Failed to an fd activity monitor", errno}; }
		}

		template<class FileDescriptorTag, class EventHandler>
		void add(
			tagged_file_descriptor_ref<FileDescriptorTag> fd_to_watch,
			activity_status initial_listen_status,
			EventHandler&& eh
		)
		{
			auto const raw_fd = fd_to_watch.native_handle();
			::epoll_event ep{
				.events = to_epoll_event(initial_listen_status),
				.data = ::epoll_data{
					.ptr = new fd_event_callback{
						[eh = std::forward<EventHandler>(eh), fd = std::move(fd_to_watch)](activity_status as){
							return eh.handle_event(as, fd.get());
						}
					}
				}
			};

			auto const res = ::epoll_ctl(
				m_epoll_fd.get(),
				EPOLL_CTL_ADD,
				raw_fd,
				&ep
			);
			if(res == -1)
			{
				delete static_cast<fd_event_callback*>(ep.data.ptr);
				throw error_handling::system_error{"Failed to add file descriptor to epoll instance", errno};
			}
		}

		void wait_for_and_distpatch_events();

	private:
		using fd_event_callback = std::move_only_function<post_process_action(activity_status)>;
		file_descriptor m_epoll_fd;
	};
}

#endif