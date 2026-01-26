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

	/**
	 * \todo Actually belongs in a json_log_reader module
	 */
	inline log::item make_log_item(jopp::object const& obj)
	{
		return log::item{
			.when = log::clock::time_point{}
				+ duration_cast<log::clock::duration>(std::chrono::duration<double>{obj.get_field_as<double>("when")}),
			.severity = log::make_severity(obj.get_field_as<std::string>("severity")),
			.message = obj.get_field_as<std::string>("message")
		};
	}

	class writer
	{
	public:
		enum class flush_result{
			keep_going,
			output_is_blocked,
			output_is_closed
		};

		explicit writer(
			size_t buffer_size = 65536,
			os_services::io::output_file_descriptor_ref initial_fd = os_services::io::output_file_descriptor_ref{STDERR_FILENO}
		):
			m_initial_fd{initial_fd},
			m_buffer_size{buffer_size},
			m_output_buffer{std::make_unique<char[]>(buffer_size)},
			m_output_write_ptr{m_output_buffer.get()},
			m_output_read_ptr{m_output_buffer.get()}
		{}

		void write(log::item const& item)
		{
			m_objects_to_write.push(to_jopp_object(item));
			if(m_initial_fd != nullptr)
			{ std::ignore = pump_data(m_initial_fd); }
		}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::io::output_file_descriptor_ref fd
		)
		{
			if(can_write(event.get_activity_status()))
			{
				m_initial_fd = nullptr;
				if(pump_data(fd) == flush_result::output_is_closed)
				{ event.stop_listening(); }
			}
		}

		[[nodiscard]] flush_result pump_data(os_services::io::output_file_descriptor_ref fd);

		[[nodiscard]] flush_result flush();

	private:
		std::queue<std::unique_ptr<jopp::object>> m_objects_to_write;
		os_services::io::output_file_descriptor_ref m_initial_fd;
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