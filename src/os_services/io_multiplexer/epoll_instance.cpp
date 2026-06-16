//@	{"target":{"name": "epoll_instance.o"}}

#include "./epoll_instance.hpp"

#include "src/os_services/error_handling/system_error.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/utils/utils.hpp"

Pipe::os_services::io_multiplexer::epoll_instance::do_add_result
Pipe::os_services::io_multiplexer::epoll_instance::do_add(
	event_handler_info const& info,
	fd::file_descriptor fd_to_watch,
	fd::activity_status initial_listening_status
)
{
	auto const prev_id = m_current_id;
	auto const eh_id = m_current_id.next();
	auto const fd = fd_to_watch.get();
	auto const insert_result = m_listeners.emplace(
		eh_id,
		epoll_entry_data{info, std::move(fd_to_watch)}
	);
	auto const event_handler = insert_result.first->second.get_header_ptr();
	assert(insert_result.second);
	::epoll_event event{
		.events = to_epoll_event(initial_listening_status),
		.data = ::epoll_data{
			.ptr = event_handler
		}
	};
	auto const epoll_insert_result = ::epoll_ctl(
		m_epoll_fd.get().native_handle(),
		EPOLL_CTL_ADD,
		fd.native_handle(),
		&event
	);
	if(epoll_insert_result == -1)
	{
		m_current_id = prev_id;
		m_listeners.erase(insert_result.first);
		throw error_handling::system_error{
			"Failed to update epoll event",
			error_handling::get_error_code()
		};
	}

	event_handler->vtable->handle_activity_event_handler_registered_event(
		event_handler + 1,
		fd::activity_event_handler_registered_event<void, fd::generic_fd_tag>{
			event_handler->fd,
			eh_id,
			fd::event_handler_cookie{event_handler},
			this
		}
	);

	return do_add_result{
		.event_handler = fd::saved_event_handler{event_handler + 1},
		.id = eh_id,
		.cookie = fd::event_handler_cookie{event_handler}
	};
}

Pipe::os_services::io_multiplexer::epoll_entry_data::epoll_entry_data(
	fd::activity_event_handler_store::event_handler_info const& eh_info,
	Pipe::os_services::fd::file_descriptor fd
)
{
	auto const struct_info = Pipe::utils::compute_struct_info(
		std::array{
			Pipe::utils::struct_field_info{
				.size = sizeof(epoll_entry_data_header),
				.alignment = alignof(epoll_entry_data_header)
			},
				Pipe::utils::struct_field_info{
				.size = eh_info.object_size,
				.alignment = eh_info.object_alignment
			}
		}
	);

	static_assert(sizeof(epoll_entry_data_header)%alignof(std::max_align_t) == 0);
	assert(eh_info.object_alignment <= alignof(std::max_align_t));

	m_storage = std::make_unique<std::byte[]>(struct_info.total_size);
	auto const storage_ptr = m_storage.get();
	auto saved_eh_info = new(storage_ptr)epoll_entry_data_header;
	saved_eh_info->handle_event = eh_info.handle_activity_event;
	saved_eh_info->vtable->fd_deleter = fd.get_deleter();
	saved_eh_info->vtable->destroy_event_handler_at = eh_info.destroy_event_handler_at;
	saved_eh_info->vtable->handle_activity_event_handler_registered_event = eh_info.handle_activity_event_handler_registered_event;
	saved_eh_info->fd = fd.release();

	eh_info.construct_event_handler_at(
		fd::activity_event_handler_store::dest_object_location{storage_ptr + sizeof(epoll_entry_data_header)},
		eh_info.object_address
	);
}

#ifdef DEBUG_RUNAWAY_EPOLL
namespace
{
	FILE* debug = nullptr;
	size_t callcount = 0;
}
#endif

void Pipe::os_services::io_multiplexer::epoll_instance::wait_for_and_distpatch_events()
{
#ifdef DEBUG_RUNAWAY_EPOLL
	if(callcount == 0)
	{
		auto filename = std::format("/dev/shm/{}.txt", getpid());
		debug = fopen(filename.c_str(), "wb");
	}
#endif

	std::array<::epoll_event, 1024> events{};
	auto const res = error_handling::do_while_eintr(
		::epoll_wait,
		m_epoll_fd.get().native_handle(),
		std::data(events),
		static_cast<int>(std::size(events)),
		-1
	);
	if(res == -1)
	{ throw error_handling::system_error{"Failed to wait for events", error_handling::get_error_code()}; }

	for(auto const& item : std::span{std::data(events), static_cast<size_t>(res)})
	{
		auto const event_handler = static_cast<epoll_entry_data_header*>(item.data.ptr);
#ifdef DEBUG_RUNAWAY_EPOLL
		if(callcount == 10000)
		{
			fclose(debug);
			abort();
		}
		fprintf(debug, "%d Status = %08x\n", event_handler->fd.native_handle(), item.events);
#endif
		event_handler->handle_event(
			event_handler + 1,  // Payload follows directly after header
			fd::activity_event<void, fd::generic_fd_tag>{
				.status = epoll_event_to_activity_status(item.events),
			}
		);
#ifdef DEBUG_RUNAWAY_EPOLL
		++callcount;
#endif
	}
}
