#ifndef PIPE_JSON_LOG_WRITER_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_WRITER_HPP

#include "src/log/log.hpp"
#include <jopp/serializer.hpp>
#include <list>

namespace Pipe::json_log_writer
{
	jopp::object to_jopp_object(log::item const& item)
	{
		jopp::object ret;
		ret.insert(
			"when",
			std::chrono::duration<double>(item.when.time_since_epoch()).count()
		);
		ret.insert("message", item.message);
		ret.insert("severity", to_string(item.severity));

		return ret;
	}

	class writer
	{
	public:
		using buffer_type = std::array<char, 8>;

		writer():
			m_output_buffer{std::make_unique<buffer_type>()},
			m_output_write_ptr{m_output_buffer->data()}
		{}

		void write(log::item const& item)
		{
			m_objects_to_write.push_back(to_jopp_object(item));
			pump_data();
		}

	private:
		std::list<jopp::object> m_objects_to_write;
		std::optional<jopp::serializer> m_current_serializer;

		void pump_data()
		{
			if(!m_current_serializer.has_value())
			{
				if(m_objects_to_write.empty())
				{ return; }

				m_current_serializer = jopp::serializer{m_objects_to_write.front()};
			}

			auto const res = m_current_serializer->serialize(*m_output_buffer);
			if(res.ec == jopp::serializer_error_code::buffer_is_full)
			{ puts("--- output buffer is full"); }
			std::ignore = ::write(STDERR_FILENO, m_output_buffer->data(), std::tuple_size_v<buffer_type>);

			switch(res.ec)
			{
				case jopp::serializer_error_code::buffer_is_full:
					// TODO: Start to flush m_output_buffer
					break;

				case jopp::serializer_error_code::completed:
					// TODO: Start to flush m_output_buffer because this is a logger
					m_current_serializer.reset();
					m_objects_to_write.pop_front();
					break;

				case jopp::serializer_error_code::illegal_char_in_string:
					throw std::runtime_error{"Invalid jopp object"};
			}
		}

		std::unique_ptr<buffer_type> m_output_buffer;
		char* m_output_write_ptr;
	};
}

#endif