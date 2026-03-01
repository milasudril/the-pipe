#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include <jopp/parser.hpp>

namespace Pipe::host
{
	class process_manager
	{
	public:
		virtual void remove(pid_t) = 0;

	private:
	};

	class client_process
	{
	public:
		struct client_ctl_tag{};
		struct log_stream_tag{};
		using proc_activity_event_handler_registered_event =
			os_services::fd::activity_event_handler_registered_event<
				void,
				os_services::proc_mgmt::pidfd_tag
			>;
		using proc_activity_event =
			os_services::fd::activity_event<
				void,
				os_services::proc_mgmt::pidfd_tag
			>;

		void handle_event(json_io::container_loaded_event<log_stream_tag>&& event);
		void handle_event(json_io::parser_error_event<log_stream_tag> event);
		void handle_event(json_io::input_closed_event<log_stream_tag>);

		void handle_event(json_io::container_loaded_event<client_ctl_tag>&& event);
		void handle_event(json_io::parser_error_event<client_ctl_tag> event);
		void handle_event(json_io::input_closed_event<client_ctl_tag>);

		void handle_event(proc_activity_event_handler_registered_event const& event)
		{ m_registration = event; }

		void handle_event(proc_activity_event);

		json_io::writer& get_ctl_output()
		{ return m_ctl_output; }

		void kill();

	private:
		json_io::writer m_ctl_output;
		proc_activity_event_handler_registered_event m_registration;
	};
}