//@	{"target":{"name":"sync_server.o"}}

#include "./sync_server.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include <type_traits>
#include <variant>

void Pipe::worker_fwk::sync_client_connection::read_and_dispatch_requests()
{
	auto const read_result = Pipe::os_services::io::read_full(
		m_registration.fd, std::span{m_input_buffer.get(), m_buffer_size}
	);
	if(read_result.bytes_transferred() == 0)
	{
		if(!read_result.operation_would_have_blocked())
		{
			// TODO: trigger connection closed
			m_registration.event_handler_store->remove(m_registration.id);
		}
		return;
	}

	auto bytes_to_process = std::span{m_input_buffer.get(), read_result.bytes_transferred()};
	while(!bytes_to_process.empty())
	{
		auto const bytes_consumed = std::visit(
			[this, bytes_to_process]<class Decoder>(Decoder& item) {
				auto const ret = item.decode(bytes_to_process);
				if(item.completed())
				{ 
					if constexpr(
						requires{
							{this->handle_request(std::move(item.get_value()), m_current_transaction_id)};
						}
					)
					{ handle_request(std::move(item.get_value()), m_current_transaction_id); }
					else
					{ handle_message(std::move(item.get_value())); }
				}
				return ret;
			},
			m_currently_received_message
		);

		bytes_to_process = std::span{
			std::begin(bytes_to_process) + bytes_consumed,
			std::end(bytes_to_process)
		};
	}
}

void Pipe::worker_fwk::sync_client_connection::enable_write_listening()
{
	if(!m_is_listening_for_write)
	{
		m_registration.event_handler_store->update_listening_status(
			m_registration.event_handler,
			os_services::fd::activity_status::read_or_write
		);
		m_is_listening_for_write = true;
	}
}

void Pipe::worker_fwk::sync_client_connection::disable_write_listening()
{
	if(m_is_listening_for_write)
	{
		m_registration.event_handler_store->update_listening_status(
			m_registration.event_handler,
			os_services::fd::activity_status::read
		);
		m_is_listening_for_write = false;
	}
}

void Pipe::worker_fwk::sync_client_connection::send_pending_messages()
{
	auto flush = [this](){
		return os_services::io::try_write(
			m_registration.fd,
			m_bytes_to_write,
			[this](os_services::io::io_blocked){ enable_write_listening(); },
			[this](os_services::io::fd_closed){
				// TODO: Notify that connection was closed
				m_registration.event_handler_store->remove(m_registration.id);
			}
		);
	};

	if(!flush())
	{ return; }

	std::span serialize_into{m_output_buffer.get(), m_buffer_size};
	size_t bytes_ready = 0;
	size_t bytes_left_to_use = m_buffer_size;
	while(!m_msgs_to_send.empty() && !serialize_into.empty())
	{
		auto const bytes_written = std::visit(
			[this, serialize_into](auto& item){
				auto const ret = item.encode(serialize_into);
				if(item.completed())
				{
					m_msgs_to_send.pop();
					// WARNING: item is dead now
				}
				return ret;
			},
			m_msgs_to_send.front()
		);

		bytes_left_to_use -= bytes_written;
		bytes_ready += bytes_written;
		serialize_into = std::span{m_output_buffer.get() + bytes_written, bytes_left_to_use};
	}

	m_bytes_to_write = std::span{static_cast<std::byte const*>(m_output_buffer.get()), bytes_ready};
	if(!flush())
	{ return; }

	if(m_msgs_to_send.empty() && m_bytes_to_write.empty())
	{ disable_write_listening(); }
}
