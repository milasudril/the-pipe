#ifndef PIPE_WORKER_CTL_WORKER_INFO_HPP
#define PIPE_WORKER_CTL_WORKER_INFO_HPP

#include <jopp/types.hpp>
#include <string>
#include <map>

namespace Pipe::worker_ctl
{
	/**
	 * \brief Contains information about an input port
	 */
	struct input_port_info
	{
	};

	/**
	 * \brief Converts a output_port_info ot a jopp::object
	 */
	inline jopp::object to_jopp_object(input_port_info const&)
	{
		jopp::object ret;
		//ret.insert("stream_content_type", obj.stream_content_type);
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a input_port_info
	 */
	inline input_port_info make_input_port_info(jopp::object const&)
	{
		return input_port_info{
		};
	}

	/**
	 * \brief A mapping from a port name to a input_port_info
	 */
	using input_port_info_map = std::map<std::string, input_port_info>;

	/**
	 * \brief Converts a input_port_info_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(input_port_info_map const& obj)
	{
		jopp::object ret;
		for(auto const& item : obj)
		{ ret.insert(jopp::string{item.first}, to_jopp_object(item.second)); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a input_port_info_map
	 */
	inline input_port_info_map make_input_port_info_map(jopp::object const& obj)
	{
		input_port_info_map ret;
		for(auto const& item: obj)
		{ ret.insert(std::pair{item.first, make_input_port_info(item.second.get<jopp::object>())}); }
		return ret;
	}

	/**
	 * \brief Contains information about an output port
	 */
	struct output_port_info
	{
		/**
		 * \brief The expected/promised content type of a port
		 * \note There is no list of valid types, but a contract should be established within a
		 * particular system
		 */
		std::string stream_content_type;
	};

	/**
	 * \brief Converts a output_port_info ot a jopp::object
	 */
	inline jopp::object to_jopp_object(output_port_info const& obj)
	{
		jopp::object ret;
		ret.insert("stream_content_type", obj.stream_content_type);
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a output_port_info
	 */
	inline output_port_info make_output_port_info(jopp::object const& obj)
	{
		return output_port_info{
			.stream_content_type = obj.get_field_as<std::string>("stream_content_type")
		};
	}

	/**
	 * \brief A mapping from a port name to a output_port_info
	 */
	using output_port_info_map = std::map<std::string, output_port_info>;

	/**
	 * \brief Converts a output_port_info_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(output_port_info_map const& obj)
	{
		jopp::object ret;
		for(auto const& item : obj)
		{ ret.insert(jopp::string{item.first}, to_jopp_object(item.second)); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a output_port_info_map
	 */
	inline output_port_info_map make_output_port_info_map(jopp::object const& obj)
	{
		output_port_info_map ret;
		for(auto const& item: obj)
		{ ret.insert(std::pair{item.first, make_output_port_info(item.second.get<jopp::object>())}); }
		return ret;
	}

	/**
	 * \brief Stores information about a worker application
	 */
	struct worker_application_info
	{
		/**
		 * \brief A user-friendly name, independent of the name of the actual binary
		 */
		std::string display_name;

		/**
		 * \brief The available input ports
		 */
		input_port_info_map inputs;

		/**
		 * \brief The available output ports
		 */
		output_port_info_map outputs;
	};

	/**
	 * \brief Converts a worker_application_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(worker_application_info const& obj)
	{
		jopp::object ret;
		ret.insert("display_name", obj.display_name);
		ret.insert("inputs", to_jopp_object(obj.inputs));
		ret.insert("outputs", to_jopp_object(obj.outputs));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a worker_application_info
	 */
	inline worker_application_info make_worker_application_info(jopp::object const& obj)
	{
		return worker_application_info{
			.display_name = obj.get_field_as<std::string>("display_name"),
			.inputs = make_input_port_info_map(obj.get_field_as<jopp::object>("inputs")),
			.outputs = make_output_port_info_map(obj.get_field_as<jopp::object>("output"))
		};
	}
}

#endif