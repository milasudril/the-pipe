//@	{"target":{"name":"writer.o"}}

#include "./writer.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"

void Pipe::json_io::writer::enable_listening()
{
	if(!m_is_listening)
	{
		m_registration.event_handler_store->update_listening_status(
			m_registration.event_handler,
			os_services::fd::activity_status::write
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
			os_services::fd::activity_status::none
		);
		m_is_listening = false;
	}
}

void Pipe::json_io::writer::handle_event(fd_ready_event const& event)
{
	if(has_error(event.status))
	{
		m_registration.event_handler_store->remove(m_registration.id);
		return;
	}

	auto flush = [this](){
		return try_write(
			m_registration.fd,
			m_reminder,
			[this](){ enable_listening(); },
			[this](){ m_registration.event_handler_store->remove(m_registration.id); }
		);
	};
	if(!flush())
	{ return; }

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

	m_reminder = std::span{static_cast<char const*>(m_output_buffer.get()), bytes_ready};
	if(!flush())
	{ return; }

	if(m_to_serialize.empty() && m_reminder.empty())
	{ disable_listening(); }
}