//@	{"dependencies_extra":[{"ref": "./activity_monitor.o", "rel": "implementation"}]}

#ifndef PIPE_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP
#define PIPE_OS_SERVICES_FD_ACTIVITY_MONITOR_HPP

#include "./file_descriptor.hpp"
#include "./activity_event.hpp"

#include "src/os_services/error_handling/error_handling.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace Pipe::os_services::fd
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

	class event_handler_id
	{
	public:
		constexpr event_handler_id() = default;

		constexpr explicit event_handler_id(uint64_t value):
			m_value{value}
		{}

		constexpr uint64_t value() const
		{ return m_value; }

		constexpr event_handler_id next()
		{
			auto ret = *this;
			++m_value;
			return ret;
		}

		constexpr bool operator==(event_handler_id const& ) const = default;

		constexpr bool operator!=(event_handler_id const& ) const = default;

	private:
		uint64_t m_value{};
	};

	struct event_handler_id_hash
	{
		static constexpr auto operator()(event_handler_id id) noexcept
		{ return std::hash<uint64_t>{}(id.value()); }
	};

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
		 * \brief This function should return the id of the event handler
		 */
		virtual event_handler_id get_id() const noexcept = 0;

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
		 */
		explicit epoll_fd_activity(
			epoll_entry_data& epoll_event_data,
			activity_status status,
			file_descriptor_ref epoll_fd
		) noexcept:
			m_epoll_event_data{epoll_event_data},
			m_status{status},
			m_epoll_fd{epoll_fd}
		{}

		void update_listening_status(activity_status new_status) const override
		{
			::epoll_event event{
				.events = to_epoll_event(new_status),
				.data = ::epoll_data{
					.ptr = &m_epoll_event_data.get()
				}
			};
			auto const result = ::epoll_ctl(
				m_epoll_fd.native_handle(),
				EPOLL_CTL_MOD,
				m_epoll_event_data.get().get_fd_native_handle(),
				&event
			);
			if(result == -1)
			{ throw error_handling::system_error{"Failed to update epoll event", errno}; }
		}

		void stop_listening() const noexcept override
		{ m_item_should_be_removed = true; }

		activity_status get_activity_status() const noexcept override
		{ return m_status; }

		/**
		 * \brief Check whether or not the event_data_should_be_deleted should be deleted
		 */
		bool item_should_be_removed() const
		{ return m_item_should_be_removed; }

		/**
		 * \brief Processes the associated event
		 */
		epoll_fd_activity process()
		{
			m_epoll_event_data.get().handle_event(*this);
			return*this;
		}

	private:
		std::reference_wrapper<epoll_entry_data> m_epoll_event_data;
		activity_status m_status;
		file_descriptor_ref m_epoll_fd;
		mutable bool m_item_should_be_removed{false};
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
		template<class T>
		requires(std::is_same_v<std::remove_cvref_t<T>, EventHandler>)
		explicit epoll_entry_data_impl(
			T&& eh,
			tagged_file_descriptor<FileDescriptorTag> fd,
			event_handler_id id
		) noexcept:
			m_event_handler{std::forward<T>(eh)},
			m_file_descriptor{std::move(fd)},
			m_id{id}
		{}

		explicit epoll_entry_data_impl(
			EventHandler&& eh,
			tagged_file_descriptor<FileDescriptorTag> fd,
			event_handler_id id
		) noexcept:
			m_event_handler{std::move(eh)},
			m_file_descriptor{std::move(fd)},
			m_id{id}
		{}

		int get_fd_native_handle() const noexcept override
		{ return m_file_descriptor.get().native_handle(); }

		void handle_event(activity_event const& event) override
		{ utils::unwrap(m_event_handler).handle_event(event, m_file_descriptor.get()); }

		event_handler_id get_id() const noexcept override
		{ return m_id; }

	private:
		EventHandler m_event_handler;
		tagged_file_descriptor<FileDescriptorTag> m_file_descriptor;
		event_handler_id m_id;
	};

	/**
	 * \brief Used to monitor activity on file descriptors
	 */
	class activity_monitor
	{
	public:
		class config_transaction
		{
		public:
			explicit config_transaction(activity_monitor& monitor):
				m_monitor{monitor}
			{}

			~config_transaction()
			{
				for(auto item : m_added_ids)
				{ m_monitor.get().remove(item); }
			}

			template<class FileDescriptorTag, activity_event_handler<FileDescriptorTag> EventHandler>
			auto& add(
				tagged_file_descriptor<FileDescriptorTag> fd_to_watch,
				activity_status initial_listen_status,
				EventHandler&& eh
			)
			{
				auto const id = m_monitor.get().add(
					std::move(fd_to_watch),
					initial_listen_status,
					std::forward<EventHandler>(eh)
				);
				m_added_ids.push_back(id);
				return *this;
			}

			void commit()
			{ m_added_ids.clear(); }

		private:
			std::reference_wrapper<activity_monitor> m_monitor;
			std::vector<event_handler_id> m_added_ids;
		};

		friend class config_transaction;

		/**
		 * \brief Constructs an activity_monitor
		 */
		activity_monitor():
			m_epoll_fd{::epoll_create1(0)}
		{
			if(m_epoll_fd == nullptr)
			{ throw error_handling::system_error{"Failed to an fd activity monitor", errno}; }
		}

		auto make_config_transaction()
		{
			return config_transaction{*this};
		}

		/**
		 * \brief Adds fd_to_watch to the activity_monitor, and starts listen for the activity_status
		 * given by initial_listen_status
		 */
		template<class FileDescriptorTag, activity_event_handler<FileDescriptorTag> EventHandler>
		[[nodiscard]] event_handler_id add(
			tagged_file_descriptor<FileDescriptorTag> fd_to_watch,
			activity_status initial_listen_status,
			EventHandler&& eh
		)
		{
			auto const id = m_current_id.next();
			auto const raw_fd = fd_to_watch.get().native_handle();
			auto const ip = m_listeners.emplace(
				id,
				std::make_unique<
					epoll_entry_data_impl<
						std::remove_cvref_t<EventHandler>,
						FileDescriptorTag
					>
				>(
					std::forward<EventHandler>(eh),
					std::move(fd_to_watch),
					id
				)
			);
			if(!ip.second)
			{ throw std::runtime_error{"File descriptor already added"}; }

			::epoll_event event{
				.events = to_epoll_event(initial_listen_status),
				.data = ::epoll_data{
					.ptr = ip.first->second.get()
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
				m_listeners.erase(ip.first);
				throw error_handling::system_error{"Failed to add file descriptor to epoll instance", errno};
			}
			return id;
		}
		void remove(event_handler_id id) noexcept
		{
			auto i = m_listeners.find(id);
			if(i == std::end(m_listeners))
			{ return; }
			::epoll_ctl(m_epoll_fd.get().native_handle(), EPOLL_CTL_DEL, i->second->get_fd_native_handle(), nullptr);
			m_listeners.erase(i);
		}

		/**
		 * \brief Waits for incoming events
		 */
		void wait_for_and_distpatch_events();

	private:
		file_descriptor m_epoll_fd;
		std::unordered_map<event_handler_id, std::unique_ptr<epoll_entry_data>, event_handler_id_hash> m_listeners;
		event_handler_id m_current_id;
	};
}

#endif