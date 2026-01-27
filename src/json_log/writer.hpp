//@	{"dependencies_extra": [{"ref": "./writer.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_LOG_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_HPP

#include "src/log/log.hpp"
#include "src/os_services/io/io.hpp"

#include <jopp/types.hpp>

namespace Pipe::json_log
{
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