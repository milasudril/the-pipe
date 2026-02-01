#ifndef PIPE_CLIENT_CTL_CLIENT_INFO_HPP
#define PIPE_CLIENT_CTL_CLIENT_INFO_HPP

#include <jopp/types.hpp>
#include <string>
#include <map>

namespace Pipe::client_ctl
{
	struct port_info
	{
		std::string type;
	};

	inline jopp::object to_jopp_object(port_info const& obj)
	{
		jopp::object ret;
		ret.insert("type", obj.type);
		return ret;
	}

	inline port_info make_port_info(jopp::object const& obj)
	{
		return port_info{
			.type = obj.get_field_as<std::string>("type")
		};
	}

	using port_info_map = std::map<std::string, port_info>;

	inline jopp::object to_jopp_object(port_info_map const& obj)
	{
		jopp::object ret;
		for(auto const& item : obj)
		{ ret.insert(jopp::string{item.first}, to_jopp_object(item.second)); }
		return ret;
	}

	inline port_info_map make_port_info_map(jopp::object const& obj)
	{
		port_info_map ret;
		for(auto const& item: obj)
		{ ret.insert(std::pair{item.first, make_port_info(item.second.get<jopp::object>())}); }
		return ret;
	}

	struct client_info
	{
		std::string display_name;
		port_info_map inputs;
		port_info_map outputs;
	};

	inline jopp::object to_jopp_object(client_info const& obj)
	{
		jopp::object ret;
		ret.insert("display_name", obj.display_name);
		ret.insert("inputs", to_jopp_object(obj.inputs));
		ret.insert("outputs", to_jopp_object(obj.outputs));
		return ret;
	}

	inline client_info make_client_info(jopp::object const& obj)
	{
		return client_info{
			.display_name = obj.get_field_as<std::string>("display_name"),
			.inputs = make_port_info_map(obj.get_field_as<jopp::object>("inputs")),
			.outputs = make_port_info_map(obj.get_field_as<jopp::object>("output"))
		};
	}
}

#endif