#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/json_rpc/json_rpc.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/client_ctl/client_application_info.hpp"

#include <filesystem>
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

		explicit client_process(
			process_manager& proc_manager,
			pid_t pid,
			std::filesystem::path&& binary
		):
			m_proc_manager{proc_manager},
			m_pid{pid},
			m_binary{std::move(binary)}
		{
			send_ctl_request(
				"get_client_application_info",
				jopp::object{},
				[this](jopp::object&& response){
					m_appinfo = client_ctl::make_client_application_info(response);
				}
			);
		}

		void handle_event(json_io::container_loaded_event<log_stream_tag>&& event);
		void handle_event(json_io::parser_error_event<log_stream_tag> event);
		void handle_event(json_io::input_closed_event<log_stream_tag>);

		void handle_event(json_io::container_loaded_event<client_ctl_tag>&& event)
		{
			std::move(event.obj).visit([this]<class T>(T&& obj){
				handle_ctl_response(std::forward<T>(obj));
			});
		}


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

		std::reference_wrapper<process_manager> m_proc_manager;
		pid_t m_pid;
		std::filesystem::path m_binary;
		client_ctl::client_application_info m_appinfo;

		void handle_ctl_response(jopp::object&& obj)
		{
			json_rpc::transaction_id id{static_cast<int64_t>(obj.get_field_as<jopp::number>("id"))};
			auto i = std::ranges::find_if(
				m_response_callbacks,
				[id](auto const& item){
					return item.first == id;
				}
			);
			if(i != std::end(m_response_callbacks))
			{
				i->second(std::move(obj.get_field_as<jopp::object>("response")));
				m_response_callbacks.erase(i);
			}
		}

		void handle_ctl_response(jopp::array&&)
		{}

		template<class Callback>
		void send_ctl_request(std::string&& method, jopp::object&& params, Callback&& cb)
		{
			auto [id, request] = m_json_rpc_ctxt.make_request(std::move(method), std::move(params));
			m_ctl_output.write(request.take_value());
			m_response_callbacks.push_back(std::pair{id, std::move(cb)});
		}

		using response_callback = std::move_only_function<void(jopp::object&&)>;
		json_rpc::context m_json_rpc_ctxt;
		std::deque<std::pair<json_rpc::transaction_id, response_callback>> m_response_callbacks;
	};
}