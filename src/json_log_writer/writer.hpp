//@	{"dependencies_extra": [{"ref": "./writer.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_LOG_WRITER_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_WRITER_HPP

#include "src/log/log.hpp"
#include "src/os_services/io/io.hpp"

#include <jopp/types.hpp>

namespace Pipe::json_log_writer
{
	inline jopp::object to_jopp_object(log::item const& item)
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
		explicit writer(
			size_t buffer_size = 65536,
			os_services::io::output_file_descriptor_ref output_fd = os_services::io::output_file_descriptor_ref{STDERR_FILENO}
		):
			m_output_fd{output_fd},
			m_buffer_size{buffer_size},
			m_output_buffer{std::make_unique<char[]>(buffer_size)}
		{}

		void write(log::item const& item);

	private:
		os_services::io::output_file_descriptor_ref m_output_fd;
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_output_buffer;
	};
}

#endif