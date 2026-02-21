#ifndef PIPE_CLIENT_APPLICATION_HPP
#define PIPE_CLIENT_APPLICATION_HPP

#include "src/json_io/reader.hpp"
#include "src/client_ctl/client_application_info.hpp"

#include <jopp/serializer.hpp>
#include <memory>

namespace Pipe::client
{
	class application
	{
	public:
		struct ctl_request_tag{};

		void handle_event(json_io::container_loaded_event<ctl_request_tag> const& event)
		{
			event.obj.visit([this](auto const& item){
					handle_request(item);
				}
			);
		}

		void handle_request(jopp::object const& object)
		{
			auto const method = object.get_field_as<std::string>("method");
			if(method == "get_client_application_info")
			{
				auto const response = to_jopp_object(get_client_application_info());
				printf("%s", to_string(response).c_str());
			}
		}

		void handle_request(jopp::value const&)
		{ throw std::runtime_error{"Unexpected request type"}; }

		void handle_request(jopp::array const& reqs)
		{
			for(auto const& item: reqs)
			{ handle_request(item); }
		}

		void handle_event(json_io::parser_error_event<ctl_request_tag>)
		{}

		static std::unique_ptr<application> create();

		virtual ~application() = default;

	private:
		virtual client_ctl::client_application_info get_client_application_info() const = 0;
	};
}

#endif