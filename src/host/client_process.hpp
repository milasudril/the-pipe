#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/os_services/io/io.hpp"
#include <jopp/parser.hpp>

namespace Pipe::host
{
	class client_process
	{
	public:
		struct client_ctl_tag{};
		struct log_stream_tag{};

		using ctl_response_event = os_services::fd::activity_event<
			client_ctl_tag,
			os_services::io::input_file_descriptor_tag
		>;

		using ctl_request_event = os_services::fd::activity_event<
			client_ctl_tag,
			os_services::io::output_file_descriptor_tag
		>;

		template<class T, class U>
		void handle_event(os_services::fd::activity_event_handler_registered_event<T, U> const&)
		{
			// TODO: handle registration events
		}

		void handle_event(json_io::container_loaded_event<log_stream_tag>&& event);
		void handle_event(json_io::parser_error_event<log_stream_tag> event);
		void handle_event(json_io::input_closed_event<log_stream_tag>);

		void handle_event(json_io::container_loaded_event<client_ctl_tag>&& event);
		void handle_event(json_io::parser_error_event<client_ctl_tag> event);
		void handle_event(json_io::input_closed_event<client_ctl_tag>);
#if 0
		void handle_event(ctl_response_event const&)
		{
		}

		void handle_event(ctl_request_event const&)
		{
		}
#endif

		json_io::writer& get_ctl_output()
		{ return m_ctl_output; }

	private:
		json_io::writer m_ctl_output;

	};
}