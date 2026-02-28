//@	{"target": {"name": "reader.o"}}

#include "./reader.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <jopp/parser.hpp>

void Pipe::json_io::reader::handle_event(data_available_event const&)
{
	std::span input_span{m_input_buffer.get(), m_buffer_size};
	auto const read_result = read_full(m_registration.fd, std::as_writable_bytes(input_span));

	if(read_result.bytes_transferred() == 0)
	{
		if(!read_result.operation_would_have_blocked())
		{
			if(m_state->parser.current_depth() != 0)
			{ m_container_receiver->handle_event(jopp::parser_error_code::more_data_needed); }

			m_registration.event_handler_store->remove(m_registration.id);
		}
		return;
	}

	input_span = std::span{std::begin(input_span), read_result.bytes_transferred()};
	while(!input_span.empty())
	{
		auto const parse_result = m_state->parser.parse(input_span);
		input_span = std::span{parse_result.ptr, std::end(input_span)};
		switch(parse_result.ec)
		{
			case jopp::parser_error_code::completed:
			{
				utils::at_scope_exit _{[this](){ m_state = std::make_unique<state>(); }};
				m_container_receiver->handle_event(std::move(m_state->container));
				break;
			}

			case jopp::parser_error_code::more_data_needed:
				assert(input_span.empty());
				break;

			default:
				m_container_receiver->handle_event(parse_result.ec);
				m_registration.event_handler_store->remove(m_registration.id);
				return;
		}
	}
}