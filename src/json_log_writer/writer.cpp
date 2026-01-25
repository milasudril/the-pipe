//@	{"target": {"name": "writer.o"}}

#include "./writer.hpp"

[[nodiscard]] Pipe::json_log_writer::writer::flush_result
Pipe::json_log_writer::writer::pump_data(os_services::io::output_file_descriptor_ref fd)
{
	if(!m_current_serializer.has_value())
	{
		if(m_objects_to_write.empty())
		{ return flush_result::keep_going; }

		m_current_serializer = jopp::serializer{*m_objects_to_write.front()};
	}

	while(!m_objects_to_write.empty())
	{
		auto const end_ptr = m_output_buffer.get() + m_buffer_size;
		auto const res = m_current_serializer->serialize(std::span{m_output_write_ptr, end_ptr});
		m_output_write_ptr = res.ptr;
		switch(res.ec)
		{
			case jopp::serializer_error_code::buffer_is_full:
				switch(flush(fd))
				{
					case flush_result::keep_going:
						m_output_write_ptr = m_output_buffer.get();
						break;
					case flush_result::output_is_blocked:
						return flush_result::output_is_blocked;
					case flush_result::output_is_closed:
						return flush_result::output_is_closed;
				}
				break;

			case jopp::serializer_error_code::completed:
				m_current_serializer.reset();
				m_objects_to_write.pop();
				switch(flush(fd))
				{
					case flush_result::keep_going:
						m_output_write_ptr = m_output_buffer.get();
						break;
					case flush_result::output_is_blocked:
						return flush_result::output_is_blocked;
					case flush_result::output_is_closed:
						return flush_result::output_is_closed;
				}
				break;
			case jopp::serializer_error_code::illegal_char_in_string:
				throw std::runtime_error{"Invalid jopp object"};
		};
	}
	return flush_result::keep_going;
}

[[nodiscard]]
Pipe::json_log_writer::writer::flush_result Pipe::json_log_writer::writer::flush(os_services::io::output_file_descriptor_ref fd)
{
	auto read_ptr = m_output_read_ptr;
	auto const write_ptr = m_output_write_ptr;
	while(read_ptr != write_ptr)
	{
		auto const result = os_services::io::write(
			fd,
			std::as_bytes(std::span{read_ptr, write_ptr})
		);

		if(result.bytes_transferred() == 0)
		{
			m_output_read_ptr = read_ptr;
			return flush_result::output_is_closed;
		}

		if(result.operation_would_have_blocked())
		{
			m_output_read_ptr = read_ptr;
			return flush_result::output_is_blocked;
		}

		read_ptr += result.bytes_transferred()/sizeof(char);
	}

	m_output_read_ptr = m_output_buffer.get();
	return flush_result::keep_going;
}