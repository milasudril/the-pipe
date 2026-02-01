//@	{"target": {"name": "reader.o"}}

#include "./reader.hpp"
#include "./item_converter.hpp"

#include <jopp/parser.hpp>

void Pipe::json_log::reader::handle_event(
	os_services::fd::activity_event const& event,
	os_services::io::input_file_descriptor_ref fd
)
{
	if(!can_read(event.get_activity_status()))
	{ return; }


	while(true)
	{
		std::span input_span{m_input_buffer.get(), m_buffer_size};
		auto const read_result = read(fd, std::as_writable_bytes(input_span));

		if(read_result.operation_would_have_blocked())
		{ return; }

		if(read_result.bytes_transferred() == 0)
		{
			if(m_state->container.empty())
			{ m_item_receiver->on_parse_error(jopp::parser_error_code::more_data_needed); }

			event.stop_listening();
			return;
		}

		input_span = std::span{m_input_buffer.get(), read_result.bytes_transferred()};
		while(true)
		{
			auto const parse_result = m_state->parser.parse(input_span);
			input_span = std::span{parse_result.ptr, read_result.bytes_transferred()};

			switch(parse_result.ec)
			{
				case jopp::parser_error_code::completed:
				{
					auto log_item = m_state->container.get_if<jopp::object>();
					if(log_item == nullptr)
					{
						m_item_receiver->on_invalid_log_item("An object expected");
						m_state = std::make_unique<state>();
						return;
					}

					auto result = make_log_item(std::move(*log_item));
					if(result.has_value())
					{ m_item_receiver->consume(std::move(*result)); }
					else
					{ m_item_receiver->on_invalid_log_item(result.error()); }

					m_state = std::make_unique<state>();
					break;
				}

				case jopp::parser_error_code::more_data_needed:
					return;

				default:
					m_item_receiver->on_parse_error(parse_result.ec);
					event.stop_listening();
					return;
			}
		}
	}
}