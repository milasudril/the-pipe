#ifndef PIPE_WORKER_APPLICATION_HPP
#define PIPE_WORKER_APPLICATION_HPP

#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/json_rpc/context.hpp"
#include "src/json_rpc/json_rpc.hpp"
#include "src/worker_ctl/connection.hpp"
#include "src/worker_ctl/worker_application_info.hpp"
#include "src/worker_ctl_json_rpc_traits/worker_ctl_json_rpc_traits.hpp"

#include <jopp/serializer.hpp>
#include <memory>

namespace Pipe::worker
{
	class application
	{
	public:
		struct ctl_request_tag{};

		void handle_event(json_io::container_loaded_event<ctl_request_tag>&& event)
		{
			std::move(event.obj).visit([this]<class T>(T&& item){
					handle_request(std::forward<T>(item));
				}
			);
		}

		void handle_request(jopp::object&& object)
		{
			json_rpc::wrapped_request request{std::move(object)};
			try
			{
				m_ctl_output.write(worker_ctl::dispatch_request(std::move(request), std::ref(*this)));
			}
			catch(std::exception const& e)
			{
				m_ctl_output.write(make_response(std::move(request), e));
			}
		}

		auto handle_request(worker_ctl::get_worker_application_info)
		{ return get_worker_application_info(); }

		json_io::writer& get_ctl_output()
		{ return m_ctl_output; }

		void handle_request(jopp::value&&)
		{ throw std::runtime_error{"Unexpected request type"}; }

		void handle_request(jopp::array&& reqs)
		{
			for(jopp::value& item: std::move(reqs))
			{
				item.visit([&]<class T>(T&& obj){
					if constexpr(std::is_same_v<T, jopp::object>)
					{ handle_request(std::forward<T>(obj)); }
					else
					{
						// TODO: Send protocol error notification "Expected object" back to client
						m_ctl_output.write(jopp::object{});
					}
				});
			}
		}

		void handle_event(json_io::parser_error_event<ctl_request_tag>)
		{}

		void handle_event(json_io::input_closed_event<ctl_request_tag>)
		{ m_should_exit = true; }

		bool should_exit() const
		{ return m_should_exit; }

		static std::unique_ptr<application> create();

		void connect(worker_ctl::input_connection&& connection)
		{
			fputs(to_string(worker_ctl::to_jopp_object(std::move(connection))).c_str(), stderr);
		}

		virtual ~application() = default;

	private:
		json_io::writer m_ctl_output;
		virtual worker_ctl::worker_application_info get_worker_application_info() const = 0;
		bool m_should_exit{false};
	};
}

#endif