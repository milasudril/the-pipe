#ifndef PIPE_JSON_LOG_WRITER_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_WRITER_HPP

#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/io/io.hpp"

#include <jopp/serializer.hpp>
#include <queue>
#include <tuple>
#include <optional>

namespace Pipe::json_log_writer
{
	std::unique_ptr<jopp::object> to_jopp_object(log::item const& item)
	{
		auto ret = std::make_unique<jopp::object>();
		ret->insert(
			"when",
			std::chrono::duration<double>(item.when.time_since_epoch()).count()
		);
		ret->insert("message", item.message);
		ret->insert("severity", to_string(item.severity));
		return ret;
	}

	class writer
	{
	public:
		using buffer_type = std::array<char, 8>;

		enum class flush_result{
			keep_going,
			output_is_blocked,
			output_is_closed
		};

		writer():
			m_output_buffer{std::make_unique<buffer_type>()},
			m_output_write_ptr{m_output_buffer->data()},
			m_output_read_ptr{m_output_buffer->data()}
		{}

		void write(log::item const& item)
		{ m_objects_to_write.push(to_jopp_object(item)); }

		void handle_event(
			os_services::fd::activity_event& event,
			os_services::io::output_file_descriptor_ref fd
		)
		{
			if(can_write(event.get_activity_status()))
			{
				if(pump_data(fd) == flush_result::output_is_closed)
				{ event.stop_listening(); }
			}
		}

		[[nodiscard]] flush_result pump_data(os_services::io::output_file_descriptor_ref fd)
		{
			if(!m_current_serializer.has_value())
			{
				if(m_objects_to_write.empty())
				{ return flush_result::keep_going; }

				m_current_serializer = jopp::serializer{*m_objects_to_write.front()};
			}

			while(!m_objects_to_write.empty())
			{
				auto const end_ptr = m_output_buffer->data() + std::tuple_size_v<buffer_type>;
				auto const res = m_current_serializer->serialize(std::span{m_output_write_ptr, end_ptr});
				m_output_write_ptr = res.ptr;
				switch(res.ec)
				{
					case jopp::serializer_error_code::buffer_is_full:
						switch(flush(fd))
						{
							case flush_result::keep_going:
								m_output_write_ptr = m_output_buffer->data();
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
								m_output_write_ptr = m_output_buffer->data();
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

		[[nodiscard]] flush_result flush(os_services::io::output_file_descriptor_ref fd)
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

			m_output_read_ptr = m_output_buffer->data();
			return flush_result::keep_going;
		}


	private:
		std::queue<std::unique_ptr<jopp::object>> m_objects_to_write;
		std::optional<jopp::serializer> m_current_serializer;

		std::unique_ptr<buffer_type> m_output_buffer;
		char* m_output_write_ptr;
		char* m_output_read_ptr;
	};
}

#endif