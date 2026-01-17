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
	/**
	 * \brief Converts an activity_status to epoll event flags
	 */
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

	/**
	 * \brief Converts epoll flags to an activity_status
	 */
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

	/**
	 * \brief Abstract base class used for data within an epoll event
	 */
	class epoll_entry_data
	{
	public:
		/**
		 * \brief This function should return the file descriptor associated with the event
		 */
		virtual int get_fd_native_handle() const noexcept = 0;

		/**
		 * \brief This function should process an activity_event
		 */
		virtual void handle_event(activity_event const& event) = 0;

		/**
		 * \brief Add virtual destructor so objects can be destructed polymorphically
		 */
		virtual ~epoll_entry_data() noexcept = default;
	};

	/**
	 * \brief Epoll specific implementation of activity_event
	 */
	class epoll_fd_activity final:public activity_event
	{
	public:
		/**
		 * \brief Constructs an epoll_fd_activity
		 * \param epoll_event_data The epoll_entry_data read from epoll_wait
		 * \param status The current file descriptor activity_status
		 * \param epoll_fd The epoll instance that issued this activity
		 *
		 * \warning Assume that the object takes ownership of epoll_event_data. The object pointed to
		 *          by epoll_event_data must have been created by new.
		 */
		explicit epoll_fd_activity(
			epoll_entry_data* epoll_event_data,
			activity_status status,
			file_descriptor_ref epoll_fd
		) noexcept:
			m_epoll_event_data{epoll_event_data},
			m_status{status},
			m_epoll_fd{epoll_fd}
		{}

		/**
		 * \brief Destructs the epoll_fd_activity
		 */
		~epoll_fd_activity()
		{
			if(m_event_data_should_be_deleted)
			{ delete m_epoll_event_data; }
		}

		/**
		 * \brief Processes the associated event
		 */
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

		void stop_listening() const noexcept override
		{
			::epoll_ctl(
				m_epoll_fd.native_handle(),
				EPOLL_CTL_DEL,
				m_epoll_event_data->get_fd_native_handle(),
				nullptr
			);
			m_event_data_should_be_deleted = true;
		}

		/**
		 * \brief Check whether or not the event_data_should_be_deleted should be deleted
		 */
		bool event_data_should_be_deleted() const
		{ return m_event_data_should_be_deleted; }

		activity_status get_activity_status() const noexcept override
		{ return m_status; }

	private:
		epoll_entry_data* m_epoll_event_data;
		activity_status m_status;
		file_descriptor_ref m_epoll_fd;
		mutable bool m_event_data_should_be_deleted{false};
	};

	/**
	 * \brief A generic implementation of epoll_entry_data
	 * \tparam EventHandler The type of activity_event_handler to use
	 * \tparam FileDescriptorTag The tag used to identify the type of file descriptor to use
	 */
	template<class EventHandler, class FileDescriptorTag>
	requires(activity_event_handler<EventHandler, FileDescriptorTag>)
	class epoll_entry_data_impl final: public epoll_entry_data
	{
	public:
		/**
		 * \brief Constructs a epoll_entry_data_impl
		 */
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

	/**
	 * \brief Used to monitor activity on file descriptors
	 */
	class activity_monitor
	{
	public:
		/**
		 * \brief Constructs an activity_monitor
		 */
		activity_monitor():
			m_epoll_fd{::epoll_create1(0)}
		{
			if(m_epoll_fd == nullptr)
			{ throw error_handling::system_error{"Failed to an fd activity monitor", errno}; }
		}

		/**
		 * \brief Adds fd_to_watch to the activity_monitor, and starts listen for the activity_status
		 * given by initial_listen_status
		 */
		template<class FileDescriptorTag, activity_event_handler<FileDescriptorTag> EventHandler>
		void add(
			tagged_file_descriptor<FileDescriptorTag> fd_to_watch,
			activity_status initial_listen_status,
			EventHandler&& eh
		)
		{
			auto const raw_fd = fd_to_watch.get().native_handle();
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

		/**
		 * \brief Waits for incoming events
		 */
		void wait_for_and_distpatch_events();

	private:
		file_descriptor m_epoll_fd;
	};
}

#endif