#ifndef PIPE_CLIENT_CTL_CLIENT_INFO_HPP
#define PIPE_CLIENT_CTL_CLIENT_INFO_HPP

#include <jopp/types.hpp>
#include <string>
#include <map>

namespace Pipe::client_ctl
{
	/**
	 * \brief Contains information about a port
	 */
	struct port_info
	{
		/**
		 * \brief The expected/promised content type of a port
		 * \note There is no list of valid types, but a contract should be established within a
		 * particular system
		 */
		std::string stream_content_type;
	};

	/**
	 * \brief Converts a port_info ot a jopp::object
	 */
	inline jopp::object to_jopp_object(port_info const& obj)
	{
		jopp::object ret;
		ret.insert("stream_content_type", obj.stream_content_type);
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a port_info
	 */
	inline port_info make_port_info(jopp::object const& obj)
	{
		return port_info{
			.stream_content_type = obj.get_field_as<std::string>("stream_content_type")
		};
	}

	/**
	 * \brief A mapping from a port name to a port_info
	 */
	using port_info_map = std::map<std::string, port_info>;

	/**
	 * \brief Converts a port_info_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(port_info_map const& obj)
	{
		jopp::object ret;
		for(auto const& item : obj)
		{ ret.insert(jopp::string{item.first}, to_jopp_object(item.second)); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a port_info_map
	 */
	inline port_info_map make_port_info_map(jopp::object const& obj)
	{
		port_info_map ret;
		for(auto const& item: obj)
		{ ret.insert(std::pair{item.first, make_port_info(item.second.get<jopp::object>())}); }
		return ret;
	}

	/**
	 * \brief Stores information about a client application
	 */
	struct client_application_info
	{
		/**
		 * \brief A user-friendly name, independent of the name of the actual binary
		 */
		std::string display_name;

		/**
		 * \brief The available input ports
		 */
		port_info_map inputs;

		/**
		 * \brief The available output ports
		 */
		port_info_map outputs;
	};

	/**
	 * \brief Converts a client_application_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(client_application_info const& obj)
	{
		jopp::object ret;
		ret.insert("display_name", obj.display_name);
		ret.insert("inputs", to_jopp_object(obj.inputs));
		ret.insert("outputs", to_jopp_object(obj.outputs));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a client_application_info
	 */
	inline client_application_info make_client_application_info(jopp::object const& obj)
	{
		return client_application_info{
			.display_name = obj.get_field_as<std::string>("display_name"),
			.inputs = make_port_info_map(obj.get_field_as<jopp::object>("inputs")),
			.outputs = make_port_info_map(obj.get_field_as<jopp::object>("output"))
		};
	}
}

#endif