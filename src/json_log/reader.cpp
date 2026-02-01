//@	{"target": {"name": "reader.o"}}

#include "./reader.hpp"
#include "./item_converter.hpp"

#include <jopp/parser.hpp>

namespace
{
	enum class parser_state{good, jammed};

	template<class State, class Receiver>
	parser_state parse_buffer(
		std::span<char const> input_span,
		std::unique_ptr<State>& state,
		Receiver& item_receiver,
		char const* who
	)
	{
		while(true)
		{
			auto const parse_result = state->parser.parse(input_span);
			auto const bytes_parsed = parse_result.ptr - std::begin(input_span);
			input_span = std::span{parse_result.ptr, std::size(input_span) - bytes_parsed};

			switch(parse_result.ec)
			{
				case jopp::parser_error_code::completed:
				{
					auto log_item = state->container.template get_if<jopp::object>();
					if(log_item == nullptr)
					{
						item_receiver.on_invalid_log_item(who, "A log item must be an object");
						state = std::make_unique<State>();
						break;
					}

					auto result = Pipe::json_log::make_log_item(std::move(*log_item));
					if(result.has_value())
					{ item_receiver.consume(who, std::move(*result)); }
					else
					{ item_receiver.on_invalid_log_item(who, result.error()); }

					state = std::make_unique<State>();
					break;
				}

				case jopp::parser_error_code::more_data_needed:
					return parser_state::good;

				default:
					item_receiver.on_parse_error(who, parse_result.ec);
					return parser_state::jammed;
			}
		}
	}
}

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
			if(m_state->parser.current_depth() != 0)
			{ m_item_receiver->on_parse_error(m_who.c_str(), jopp::parser_error_code::more_data_needed); }

			event.stop_listening();
			return;
		}

		switch(
			parse_buffer(
				std::span{std::begin(input_span), read_result.bytes_transferred()},
				m_state,
				*m_item_receiver,
				m_who.c_str()
			)
		)
		{
			case parser_state::good:
				break;
			case parser_state::jammed:
				event.stop_listening();
				return;
		}
	}
}