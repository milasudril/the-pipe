//@	{"target": {"name": "reader.o"}}

#include "./reader.hpp"

#include <jopp/parser.hpp>

namespace
{
	enum class parser_state{good, jammed};

	template<class State, class Receiver>
	parser_state parse_buffer(
		std::span<char const> input_span,
		std::unique_ptr<State>& state,
		Receiver& item_receiver
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
					Pipe::utils::at_scope_exit _{[&state](){ state = std::make_unique<State>(); }};
					item_receiver.handle_event(std::move(state->container));
					break;
				}

				case jopp::parser_error_code::more_data_needed:
					return parser_state::good;

				default:
					item_receiver.handle_event(parse_result.ec);
					return parser_state::jammed;
			}
		}
	}
}

void Pipe::json_io::reader::handle_event(
	os_services::fd::activity_event_handler_store& source,
	event_type const& event
)
{
	if(!can_read(event.status))
	{ return; }

	while(true)
	{
		std::span input_span{m_input_buffer.get(), m_buffer_size};
		auto const read_result = read(event.fd, std::as_writable_bytes(input_span));

		if(read_result.operation_would_have_blocked())
		{ return; }

		if(read_result.bytes_transferred() == 0)
		{
			if(m_state->parser.current_depth() != 0)
			{ m_container_receiver->handle_event(jopp::parser_error_code::more_data_needed); }

			source.remove(event.event_handler);
			return;
		}

		switch(
			parse_buffer(
				std::span{std::begin(input_span), read_result.bytes_transferred()},
				m_state,
				*m_container_receiver
			)
		)
		{
			case parser_state::good:
				break;
			case parser_state::jammed:
				source.remove(event.event_handler);
				return;
		}
	}
}