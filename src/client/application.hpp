#ifndef PIPE_CLIENT_APPLICATION_HPP
#define PIPE_CLIENT_APPLICATION_HPP

#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/client_ctl/client_application_info.hpp"

#include <jopp/serializer.hpp>
#include <memory>

namespace Pipe::client
{
	// TODO: Should be put in a json_rpc helper module
	inline void set_jsonrpc_fields(jopp::object&& request, jopp::object& response)
	{
		if(auto id = request.find("id"); id != std::end(request))
		{ response.insert("id", jopp::value{std::move(id->second)}); }
		response.insert("jsonrpc", "2.0");
	}

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
			auto const method = object.get_field_as<std::string>("method");
			if(method == "get_client_application_info")
			{
				jopp::object response;
				response.insert("result", to_jopp_object(get_client_application_info()));
				set_jsonrpc_fields(std::move(object), response);
				m_ctl_output.write(jopp::container{std::move(response)});
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

		static std::unique_ptr<application> create();

		virtual ~application() = default;

	private:
		json_io::writer m_ctl_output;
		virtual client_ctl::client_application_info get_client_application_info() const = 0;
	};
}

#endif