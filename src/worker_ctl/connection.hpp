#ifndef PIPE_WORKER_CTL_CONNECTION_HPP
#define PIPE_WORKER_CTL_CONNECTION_HPP

#include "src/worker_ctl/data_format_info.hpp"
#include "src/os_services/fs/file_open_precondition.hpp"
#include "src/os_services/fs/file_access_permission.hpp"

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	struct endpoint_path
	{
		std::string type;
		jopp::value value;
	};

	inline jopp::object to_jopp_object(endpoint_path&& obj)
	{
		jopp::object ret;
		ret.insert("type", std::move(obj.type));
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	inline endpoint_path make_endpoint_path(jopp::object&& obj)
	{
		auto value = obj.find("value");
		if(value == std::end(obj))
		{ throw std::runtime_error{"Missing mandatory field `value`"}; }

		return endpoint_path{
			.type = std::move(obj.get_field_as<jopp::string>("type")),
			.value = std::move(value->second)
		};
	}

	struct remote_output_endpoint
	{
		endpoint_path endpoint;
		data_format_info provides;
	};

	inline jopp::object to_jopp_object(remote_output_endpoint&& obj)
	{
		jopp::object ret;
		ret.insert("endpoint", to_jopp_object(std::move(obj.endpoint)));
		ret.insert("provides", to_jopp_object(std::move(obj.provides)));
		return ret;
	}

	inline remote_output_endpoint make_remote_output_endpoint(jopp::object&& obj)
	{
		return remote_output_endpoint{
			.endpoint = make_endpoint_path(std::move(obj.get_field_as<jopp::object>("endpoint"))),
			.provides = make_data_format_info(std::move(obj.get_field_as<jopp::object>("provides")))
		};
	}

	struct input_connection
	{
		remote_output_endpoint endpoint;
		std::string portname;
	};

	inline jopp::object to_jopp_object(input_connection&& obj)
	{
		jopp::object ret;
		ret.insert("endpoint", to_jopp_object(std::move(obj.endpoint)));
		ret.insert("portname", std::move(obj.portname));
		return ret;
	}

	inline input_connection make_input_connection(jopp::object&& obj)
	{
		return input_connection{
			.endpoint = make_remote_output_endpoint(std::move(obj.get_field_as<jopp::object>("endpoint"))),
			.portname = std::move(obj.get_field_as<jopp::string>("portname"))
		};
	}

	struct remote_input_endpoint
	{
		endpoint_path path;
	};

	inline jopp::object to_jopp_object(remote_input_endpoint&& obj)
	{
		jopp::object ret;
		ret.insert("path", to_jopp_object(std::move(obj.path)));
		return ret;
	}

	inline remote_input_endpoint make_remote_input_endpoint(jopp::object&& obj)
	{
		return remote_input_endpoint{
			.path = make_endpoint_path(std::move(obj.get_field_as<jopp::object>("path")))
		};
	}

	struct endpoint_open_opts
	{
		os_services::fs::file_open_precondition precond{os_services::fs::file_open_precondition::none};
		os_services::fs::file_access_permission created_endpoint_perms{
			os_services::fs::file_access_permission::owner_read | os_services::fs::file_access_permission::owner_write
		};
	};

	struct output_connection
	{
		remote_input_endpoint endpoint;
		std::string portname;
	};

	inline jopp::object to_jopp_object(output_connection&& obj)
	{
		jopp::object ret;
		ret.insert("endpoint", to_jopp_object(std::move(obj.endpoint)));
		ret.insert("portname", std::move(obj.portname));
		return ret;
	}

	inline output_connection mkae_output_connection(jopp::object&& obj)
	{
		return output_connection{
			.endpoint = make_remote_input_endpoint(std::move(obj.get_field_as<jopp::object>("endpoint"))),
			.portname = std::move(obj.get_field_as<jopp::string>("portname"))
		};
	}
}
#endif