//@	{"dependencies_extra":[{"ref": "./epoll_instance.o", "rel": "implementation"}]}

#ifndef PIPE_OS_SERVICES_IO_MULTIPLEXER_EPOLL_INSTANCE_HPP
#define PIPE_OS_SERVICES_IO_MULTIPLEXER_EPOLL_INSTANCE_HPP

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/log/log.hpp"
#include "src/os_services/error_handling/error_handling.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace Pipe::os_services::io_multiplexer
{
	/**
	 * \brief Converts an activity_status to epoll event flags
	 */
	constexpr unsigned int to_epoll_event(fd::activity_status status)
	{
		switch(status)
		{
			case fd::activity_status::none:
				return 0;
			case fd::activity_status::read:
				return EPOLLIN;
			case fd::activity_status::write:
				return EPOLLOUT;
			case fd::activity_status::read_or_write:
				return EPOLLIN|EPOLLOUT;
			default:
				throw std::runtime_error{"Bad activity status"};
		}
		throw std::runtime_error{"Bad activity status"};
	}

	/**
	 * \brief Converts epoll flags to an activity_status
	 */
	constexpr fd::activity_status epoll_event_to_activity_status(unsigned int event)
	{
		if((event&EPOLLERR) || (event&EPOLLHUP) || (event&EPOLLRDHUP))
		{ return fd::activity_status::error; }

		if((event & EPOLLIN) && (event & EPOLLOUT))
		{ return fd::activity_status::read_or_write; }

		if(event & EPOLLIN)
		{ return fd::activity_status::read; }

		if(event & EPOLLOUT)
		{ return fd::activity_status::write; }

		return fd::activity_status::none;
	}

	/**
	 * \brief Storage for less frequently used function pointers in epoll_entry_data_header
	 */
	struct epoll_entry_data_vtable
	{
		fd::file_descriptor_deleter<fd::generic_fd_tag> fd_deleter;
		void (*destroy_event_handler_at)(void* object);
	};

	/**
	 * \brief The header used stored in front of the event handler in the epoll_entry_data
	 */
	struct alignas(16) epoll_entry_data_header
	{
		epoll_entry_data_header():vtable{std::make_unique<epoll_entry_data_vtable>()}
		{}

		void (*handle_event)(
			void* object,
			fd::activity_event<void, fd::generic_fd_tag> const& event
		);

		fd::file_descriptor_ref fd;
		std::unique_ptr<epoll_entry_data_vtable> vtable;
	};

	/**
	 * \brief Class containing data associated with an epoll registration
	 */
	class epoll_entry_data
	{
	public:
		epoll_entry_data() = default;
		epoll_entry_data(epoll_entry_data&&) = default;
		epoll_entry_data& operator=(epoll_entry_data&&) = default;
		epoll_entry_data(epoll_entry_data const&) = delete;
		epoll_entry_data& operator=(epoll_entry_data const&) = delete;

		~epoll_entry_data()
		{
			if(m_storage != nullptr)
			{
				auto header = get_header_ptr();
				header->vtable->destroy_event_handler_at(get_event_handler_ptr());
				header->vtable->fd_deleter(header->fd);
				header->~epoll_entry_data_header();
			}
		}


	explicit epoll_entry_data(
		fd::activity_event_handler_store::event_handler_info const& eh_info,
		Pipe::os_services::fd::file_descriptor fd
	);

	epoll_entry_data_header* get_header_ptr()
	{ return reinterpret_cast<epoll_entry_data_header*>(m_storage.get()); }

	void* get_event_handler_ptr() const
	{ return m_storage.get() + sizeof(epoll_entry_data_header); }

	private:
		std::unique_ptr<std::byte[]> m_storage;
	};


	struct epoll_fd_tag
	{};

	using epoll_file_descriptor_ref = fd::tagged_file_descriptor_ref<epoll_fd_tag>;

	using epoll_file_descriptor = fd::tagged_file_descriptor<epoll_fd_tag>;

	class epoll_instance:public fd::activity_event_handler_store
	{
	public:
		void wait_for_and_distpatch_events();

		epoll_instance():
			m_epoll_fd{::epoll_create1(0)}
		{
			if(!m_epoll_fd)
			{
				throw error_handling::system_error{
					"Failed to create a new epoll instance", error_handling::get_error_code()
				};
			}
		}

		void update_listening_status(
			Pipe::os_services::fd::event_handler_cookie handle,
			Pipe::os_services::fd::activity_status new_status
		) noexcept override
		{
			auto const event_handler = static_cast<epoll_entry_data_header const*>(handle.get());
			::epoll_event event{
				.events = to_epoll_event(new_status),
				.data = ::epoll_data{
					.ptr = handle.get()
				}
			};
			auto const result = ::epoll_ctl(
				m_epoll_fd.get().native_handle(),
				EPOLL_CTL_MOD,
				event_handler->fd.native_handle(),
				&event
			);
			if(result == -1)
			{ Pipe::log::terminate_with_message("epoll_ctl EPOLL_CTL_MOD failed"); }
		}

		bool is_empty() const
		{ return m_listeners.empty(); }

		size_t get_num_listeners() const
		{ return std::size(m_listeners); }

	private:
		void remove(fd::event_handler_id event_handler) noexcept override
		{
			auto const i = m_listeners.find(event_handler);
			if(i == std::end(m_listeners))
			{ return; }
			::epoll_ctl(
				m_epoll_fd.get().native_handle(),
				EPOLL_CTL_DEL,
				i->second.get_header_ptr()->fd,
				nullptr
			);
			m_listeners.erase(i);
		}

		do_add_result do_add(
			event_handler_info const& info,
			fd::file_descriptor fd_to_watch,
			fd::activity_status initial_listening_status
		) override;

		epoll_file_descriptor m_epoll_fd;
		std::unordered_map<fd::event_handler_id, epoll_entry_data, fd::event_handler_id_hash> m_listeners;
		fd::event_handler_id m_current_id;
	};

}

#endif
