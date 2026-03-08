#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/json_rpc/json_rpc.hpp"
#include "src/json_rpc/context.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/worker_ctl/worker_application_info.hpp"
#include "src/worker_ctl/connection.hpp"

#include <filesystem>
#include <jopp/parser.hpp>

namespace Pipe::json_rpc
{
	template<>
	struct request_traits<worker_ctl::get_worker_application_info>
	{
		static constexpr char const* method = "get_worker_application_info";

		static jopp::object params(worker_ctl::get_worker_application_info)
		{ return jopp::object{}; }

		static worker_ctl::worker_application_info make_response(jopp::value&& val)
		{ return worker_ctl::make_worker_application_info(std::move(val.get<jopp::object>())); }
	};
}

namespace Pipe::host
{
	class process_manager
	{
	public:
		virtual void remove(pid_t) = 0;

	private:
	};

	class worker_process
	{
	public:
		struct worker_ctl_tag{};
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

		explicit worker_process(
			process_manager& proc_manager,
			pid_t pid,
			std::filesystem::path&& binary
		):
			m_proc_manager{proc_manager},
			m_pid{pid},
			m_binary{std::move(binary)}
		{
			m_json_rpc_ctxt.send_request(
				std::ref(m_ctl_output),
				worker_ctl::get_worker_application_info{},
				[this](worker_ctl::worker_application_info&& response){
					m_appinfo = std::move(response);
				}
			);
		}

		void handle_event(json_io::container_loaded_event<log_stream_tag>&& event);
		void handle_event(json_io::parser_error_event<log_stream_tag> event);
		void handle_event(json_io::input_closed_event<log_stream_tag>);

		void handle_event(json_io::container_loaded_event<worker_ctl_tag>&& event)
		{
			m_json_rpc_ctxt.handle_messages(std::move(event.obj), std::ref(*this));
		}

		template<class Tag>
		void handle_json_rpc_notification(jopp::object&&)
		{}


		void handle_event(json_io::parser_error_event<worker_ctl_tag> event);
		void handle_event(json_io::input_closed_event<worker_ctl_tag>);

		void handle_event(proc_activity_event_handler_registered_event const& event)
		{ m_registration = event; }

		void handle_event(proc_activity_event);

		json_io::writer& get_ctl_output()
		{ return m_ctl_output; }

		void kill();

	private:
		json_io::writer m_ctl_output;
		proc_activity_event_handler_registered_event m_registration;

		std::reference_wrapper<process_manager> m_proc_manager;
		pid_t m_pid;
		std::filesystem::path m_binary;
		worker_ctl::worker_application_info m_appinfo;
		json_rpc::context m_json_rpc_ctxt;
	};
}