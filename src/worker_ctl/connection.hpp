#ifndef PIPE_WORKER_CTL_CONNECTION_HPP
#define PIPE_WORKER_CTL_CONNECTION_HPP

#include "src/worker_ctl/data_stream_info.hpp"
#include "src/os_services/fs/file_open_precondition.hpp"
#include "src/os_services/fs/file_access_permission.hpp"

#include <jopp/types.hpp>
#include <ranges>

namespace Pipe::worker_ctl
{
	struct endpoint_path_tag{};

	using endpoint_path = any<endpoint_path_tag>;

	inline endpoint_path make_endpoint_path(jopp::object&& obj)
	{ return make_any<endpoint_path_tag>(std::move(obj)); }

	struct remote_output_endpoint
	{
		endpoint_path address;
		data_stream_info provides;
	};

	inline jopp::object to_jopp_object(remote_output_endpoint&& obj)
	{
		jopp::object ret;
		ret.insert("address", to_jopp_object(std::move(obj.address)));
		ret.insert("provides", to_jopp_object(std::move(obj.provides)));
		return ret;
	}

	inline remote_output_endpoint make_remote_output_endpoint(jopp::object&& obj)
	{
		return remote_output_endpoint{
			.address = make_endpoint_path(std::move(obj.get_field_as<jopp::object>("address"))),
			.provides = make_data_stream_info(std::move(obj.get_field_as<jopp::object>("provides")))
		};
	}

	struct input_connection
	{
		remote_output_endpoint remote_endpoint;
		std::string portname;
	};

	inline jopp::object to_jopp_object(input_connection&& obj)
	{
		jopp::object ret;
		ret.insert("remote_endpoint", to_jopp_object(std::move(obj.remote_endpoint)));
		ret.insert("portname", std::move(obj.portname));
		return ret;
	}

	inline input_connection make_input_connection(jopp::object&& obj)
	{
		return input_connection{
			.remote_endpoint = make_remote_output_endpoint(
				std::move(obj.get_field_as<jopp::object>("remote_endpoint"))
			),
			.portname = std::move(obj.get_field_as<jopp::string>("portname"))
		};
	}

	struct remote_input_endpoint
	{
		endpoint_path address;
	};

	inline jopp::object to_jopp_object(remote_input_endpoint&& obj)
	{
		jopp::object ret;
		ret.insert("address", to_jopp_object(std::move(obj.address)));
		return ret;
	}

	inline remote_input_endpoint make_remote_input_endpoint(jopp::object&& obj)
	{
		return remote_input_endpoint{
			.address = make_endpoint_path(std::move(obj.get_field_as<jopp::object>("address")))
		};
	}

	struct endpoint_open_opts
	{
		os_services::fs::file_open_precondition precond{os_services::fs::file_open_precondition::none};
		os_services::fs::file_access_permission created_endpoint_perms{
			os_services::fs::file_access_permission::owner_read | os_services::fs::file_access_permission::owner_write
		};
	};

	inline jopp::object to_jopp_object(endpoint_open_opts const& opts)
	{
		jopp::object ret;
		ret.insert("precond", to_string(opts.precond));
		ret.insert(
			"created_endpoint_perms",
			to_array_of_strings<jopp::array>(opts.created_endpoint_perms)
		);

		return ret;
	}

	inline endpoint_open_opts make_endpoint_open_opts(jopp::object const& obj)
	{
		endpoint_open_opts ret;
		auto const precond = obj.try_get_field_as<jopp::string>("precond");
		if(precond != nullptr)
		{ ret.precond = os_services::fs::make_file_open_precondition(*precond); }

		auto const created_endpoint_perms_jopp = obj.try_get_field_as<jopp::array>("created_endpoint_perms");
		if(created_endpoint_perms_jopp != nullptr)
		{
			ret.created_endpoint_perms = os_services::fs::make_file_access_permission(
				std::ranges::transform_view{
					*created_endpoint_perms_jopp,
					[](jopp::value const& item){
						return item.get<jopp::string>();
					}
				}
			);
		}

		return ret;
	}

	struct output_connection
	{
		remote_input_endpoint remote_endpoint;
		std::string portname;
		endpoint_open_opts remote_endpoint_open_opts;
	};

	inline jopp::object to_jopp_object(output_connection&& obj)
	{
		jopp::object ret;
		ret.insert("remote_endpoint", to_jopp_object(std::move(obj.remote_endpoint)));
		ret.insert("portname", std::move(obj.portname));
		ret.insert("remote_endpoint_open_opts", to_jopp_object(obj.remote_endpoint_open_opts));
		return ret;
	}

	inline output_connection make_output_connection(jopp::object&& obj)
	{
		output_connection ret;
		ret.remote_endpoint =
			make_remote_input_endpoint(std::move(obj.get_field_as<jopp::object>("remote_endpoint"))),
		ret.portname = std::move(obj.get_field_as<jopp::string>("portname"));

		auto const remote_endpoint_open_opts = obj.try_get_field_as<jopp::object>("remote_endpoint_open_opts");
		if(remote_endpoint_open_opts != nullptr)
		{ ret.remote_endpoint_open_opts = make_endpoint_open_opts(*remote_endpoint_open_opts); }

		return ret;
	}
}
#endif