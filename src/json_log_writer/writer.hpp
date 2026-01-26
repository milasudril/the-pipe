//@	{"dependencies_extra": [{"ref": "./writer.o", "rel":"implementation"}]}

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
	inline std::unique_ptr<jopp::object> to_jopp_object(log::item const& item)
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
		enum class flush_result{
			keep_going,
			output_is_blocked,
			output_is_closed
		};

		explicit writer(size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_output_buffer{std::make_unique<char[]>(buffer_size)},
			m_output_write_ptr{m_output_buffer.get()},
			m_output_read_ptr{m_output_buffer.get()}
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

		[[nodiscard]] flush_result pump_data(os_services::io::output_file_descriptor_ref fd);

		[[nodiscard]] flush_result flush();

	private:
		std::queue<std::unique_ptr<jopp::object>> m_objects_to_write;
		struct current_state
		{
			jopp::serializer serializer;
			os_services::io::output_file_descriptor_ref fd;
		};
		std::optional<current_state> m_current_state;

		size_t m_buffer_size;
		std::unique_ptr<char[]> m_output_buffer;
		char* m_output_write_ptr;
		char* m_output_read_ptr;
	};
}

#endif