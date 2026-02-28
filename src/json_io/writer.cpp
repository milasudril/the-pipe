//@	{"target":{"name":"writer.o"}}

#include "./writer.hpp"
#include "src/os_services/io/io.hpp"

void Pipe::json_io::writer::enable_listening()
{
	if(!m_is_listening)
	{
		m_registration.event_handler_store->update_listening_status(
			m_registration.event_handler,
			os_services::fd::activity_status::read_or_write
		);
		m_is_listening = true;
	}
}

void Pipe::json_io::writer::disable_listening()
{
	if(m_is_listening)
	{
		m_registration.event_handler_store->update_listening_status(
			m_registration.event_handler,
			os_services::fd::activity_status::read
		);
		m_is_listening = false;
	}
}

void Pipe::json_io::writer::handle_event(fd_ready_event const&)
{
	if(!m_reminder.empty())
	{
		auto const write_result = write_full(m_registration.fd, std::as_bytes(m_reminder));
		m_reminder = std::span{
			std::begin(m_reminder) + write_result.bytes_transferred(), std::end(m_reminder)
		};
		if(write_result.operation_would_have_blocked())
		{
			enable_listening();
			return;
		}

		if(write_result.bytes_transferred() == 0)
		{
			m_registration.event_handler_store->remove(m_registration.id);
			return;
		}
	}

	assert(m_reminder.empty());

	std::span serialize_into{m_output_buffer.get(), m_buffer_size};
	size_t bytes_ready = 0;
	size_t bytes_left_to_use = m_buffer_size;
	while(!m_to_serialize.empty() && !serialize_into.empty())
	{
		auto item = m_to_serialize.front().get();
		auto const serialization_result = item->serialize_to(serialize_into);
		auto const chars_written = serialization_result.ptr - std::data(serialize_into);
		bytes_left_to_use -= chars_written;
		bytes_ready += chars_written;
		serialize_into = std::span{serialization_result.ptr, bytes_left_to_use};
		switch(serialization_result.ec)
		{
			case jopp::serializer_error_code::buffer_is_full:
				assert(serialize_into.empty());
				break;

			case jopp::serializer_error_code::completed:
				m_to_serialize.pop();
				break;

			case jopp::serializer_error_code::illegal_char_in_string:
				m_registration.event_handler_store->remove(m_registration.id);
				return;
		}
	}

	std::span write_from{static_cast<char const*>(m_output_buffer.get()), bytes_ready};
	if(!write_from.empty())
	{
		auto const write_result = write_full(m_registration.fd, std::as_bytes(write_from));
		m_reminder = std::span{
			std::begin(write_from) + write_result.bytes_transferred(),
			std::end(write_from)
		};
		if(write_result.operation_would_have_blocked())
		{
			enable_listening();
			return;
		}

		if(write_result.bytes_transferred() == 0)
		{
			m_registration.event_handler_store->remove(m_registration.id);
			return;
		}
	}

	if(m_to_serialize.empty() && m_reminder.empty())
	{ disable_listening(); }
}