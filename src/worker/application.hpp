#ifndef PIPE_WORKER_APPLICATION_HPP
#define PIPE_WORKER_APPLICATION_HPP

#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/worker_ctl/worker_application_info.hpp"
#include "src/json_rpc/json_rpc.hpp"

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
			auto const method = request.method();

			if(method == "get_worker_application_info")
			{
				m_ctl_output.write(
					make_response(std::move(request), to_jopp_object(get_worker_application_info()))
				);
			}
			else
			{
				throw std::runtime_error{"Unsupported method"};
			}
		}

		json_io::writer& get_ctl_output()
		{ return m_ctl_output; }

		void handle_request(jopp::value&&)
		{ throw std::runtime_error{"Unexpected request type"}; }

		void handle_request(jopp::array&& reqs)
		{
			for(jopp::value& item: std::move(reqs))
			{ handle_request(std::move(item.get<jopp::object>())); }
		}

		void handle_event(json_io::parser_error_event<ctl_request_tag>)
		{}

		void handle_event(json_io::input_closed_event<ctl_request_tag>)
		{ m_should_exit = true; }

		bool should_exit() const
		{ return m_should_exit; }

		static std::unique_ptr<application> create();

		virtual ~application() = default;

	private:
		json_io::writer m_ctl_output;
		virtual worker_ctl::worker_application_info get_worker_application_info() const = 0;
		bool m_should_exit{false};
	};
}

#endif