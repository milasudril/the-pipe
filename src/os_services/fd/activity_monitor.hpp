//@	{"dependencies_extra":[{"ref": "./activity_monitor.o", "rel": "implementation"}]}

#ifndef PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP
#define PROG_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP

#include "./file_descriptor.hpp"
#include "./activity_event.hpp"

#include "src/os_services/error_handling/error_handling.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/epoll.h>

namespace prog::os_services::fd
{
	constexpr unsigned int to_epoll_event(activity_status status)
	{
		switch(status)
		{
			case activity_status::none:
				return 0;
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

		return activity_status::none;
	}

	class epoll_entry_data
	{
	public:
		virtual int get_fd_native_handle() const noexcept = 0;
		virtual void handle_event(activity_event const& event) = 0;
		virtual ~epoll_entry_data() noexcept = default;
	};

	class epoll_fd_activity:public activity_event
	{
	public:
		explicit epoll_fd_activity(
			epoll_entry_data* epoll_event_data,
			activity_status status,
			file_descriptor_ref epoll_fd
		) noexcept:
			m_epoll_event_data{epoll_event_data},
			m_status{status},
			m_epoll_fd{epoll_fd}
		{}

		void process()
		{ m_epoll_event_data->handle_event(*this); }

		void update_listening_status(activity_status new_status) const override
		{
			::epoll_event event{
				.events = to_epoll_event(new_status),
				.data = ::epoll_data{
					.ptr = m_epoll_event_data
				}
			};
			auto const result = ::epoll_ctl(
				m_epoll_fd.native_handle(),
				EPOLL_CTL_MOD,
				m_epoll_event_data->get_fd_native_handle(),
				&event
			);
			if(result == -1)
			{ throw error_handling::system_error{"Failed to update epoll event", errno}; }
		}

		void close_fd() const noexcept override
		{ delete m_epoll_event_data; }

		activity_status get_activity_status() const noexcept override
		{ return m_status; }

	private:
		epoll_entry_data* m_epoll_event_data;
		activity_status m_status;
		file_descriptor_ref m_epoll_fd;
	};

	template<activity_event_handler EventHandler, class FileDescriptorTag>
	class epoll_entry_data_impl: public epoll_entry_data
	{
	public:
		explicit epoll_entry_data_impl(
			EventHandler&& eh,
			tagged_file_descriptor<FileDescriptorTag> fd
		) noexcept:
			m_event_handler{std::move(eh)},
			m_file_descriptor{std::move(fd)}
		{}

		int get_fd_native_handle() const noexcept override
		{ return m_file_descriptor.get().native_handle(); }

		void handle_event(activity_event const& event) override
		{ m_event_handler.handle_event(event, m_file_descriptor.get()); }

	private:
		EventHandler m_event_handler;
		tagged_file_descriptor<FileDescriptorTag> m_file_descriptor;
	};

	class activity_monitor
	{
	public:
		activity_monitor():
			m_epoll_fd{::epoll_create1(0)}
		{
			if(m_epoll_fd == nullptr)
			{ throw error_handling::system_error{"Failed to an fd activity monitor", errno}; }
		}

		template<class FileDescriptorTag, activity_event_handler EventHandler>
		void add(
			tagged_file_descriptor_ref<FileDescriptorTag> fd_to_watch,
			activity_status initial_listen_status,
			EventHandler&& eh
		)
		{
			auto const raw_fd = fd_to_watch.native_handle();
			::epoll_event event{
				.events = to_epoll_event(initial_listen_status),
				.data = ::epoll_data{
					.ptr = new epoll_entry_data_impl{std::move(eh), std::move(fd_to_watch)}
				}
			};

			auto const res = ::epoll_ctl(
				m_epoll_fd.get(),
				EPOLL_CTL_ADD,
				raw_fd,
				&event
			);
			if(res == -1)
			{
				delete static_cast<epoll_entry_data*>(event.data.ptr);
				throw error_handling::system_error{"Failed to add file descriptor to epoll instance", errno};
			}
		}

		void wait_for_and_distpatch_events();

	private:
		file_descriptor m_epoll_fd;
	};
}

#endif