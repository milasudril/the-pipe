//@	{"target":{"name":"sync_server.o"}}

#include "./sync_server.hpp"
#include "src/os_services/io/io.hpp"

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
				{ handle_request(std::move(item.get_value())); }
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

void Pipe::worker_fwk::sync_client_connection::send_pending_responses()
{}