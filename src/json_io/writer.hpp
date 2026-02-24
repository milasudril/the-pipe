//@	{"dependencies_extra": [{"ref": "./writer.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_IO_WRITER_HPP
#define PIPE_JSON_IO_WRITER_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <jopp/types.hpp>
#include <jopp/serializer.hpp>
#include <memory>

namespace Pipe::json_io
{
	class writer
	{
	public:
		struct json_stream_tag{};
		using fd_ready_event = os_services::fd::activity_event<
			json_stream_tag,
			os_services::io::output_file_descriptor_tag
		>;

		using activity_event_handler_registered_event = os_services::fd::activity_event_handler_registered_event<
			json_stream_tag,
			os_services::io::output_file_descriptor_tag
		>;

		explicit writer(size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_output_buffer{std::make_unique<char[]>(buffer_size)}
		{}

		void handle_event(fd_ready_event const& event);

		void write(jopp::container&& item_to_write);

		void handle_event(
			activity_event_handler_registered_event const& event
		)
		{
			m_registration = event;
		}

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_output_buffer;
		activity_event_handler_registered_event m_registration;
	};
}

#endif