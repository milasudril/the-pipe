#ifndef PIPE_WORKER_CTL_WORKER_INFO_HPP
#define PIPE_WORKER_CTL_WORKER_INFO_HPP

#include "./data_stream_info.hpp"

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
		std::vector<port_capability> accepts;
	};

	/**
	 * \brief Converts a output_port_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(input_port_info&& obj)
	{
		jopp::object ret;

		jopp::array accepts;
		for(auto&& item : std::move(obj.accepts))
		{ accepts.push_back(to_jopp_object(std::move(item))); }
		ret.insert("accepts", std::move(accepts));

		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a input_port_info
	 */
	inline input_port_info make_input_port_info(jopp::object&& obj)
	{
		std::vector<port_capability> accepts;
		for(auto&& item: std::move(obj.get_field_as<jopp::array>("accepts")))
		{ accepts.push_back(make_port_capability(std::move(item.get<jopp::object>()))); }

		return input_port_info{
			.accepts = std::move(accepts)
		};
	}

	/**
	 * \brief A mapping from a port name to a input_port_info
	 */
	using input_port_info_map = std::map<std::string, input_port_info>;

	/**
	 * \brief Converts a input_port_info_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(input_port_info_map&& obj)
	{
		jopp::object ret;
		for(auto&& item : std::move(obj))
		{ ret.insert(jopp::string{item.first}, to_jopp_object(std::move(item.second))); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a input_port_info_map
	 */
	inline input_port_info_map make_input_port_info_map(jopp::object&& obj)
	{
		input_port_info_map ret;
		for(auto&& item: std::move(obj))
		{
			ret.insert(
				std::pair{
					item.first,
					make_input_port_info(std::move(item.second.get<jopp::object>()))
				}
			);
		}
		return ret;
	}

	/**
	 * \brief Contains information about an output port
	 */
	struct output_port_info
	{
		port_capability provides;
	};

	/**
	 * \brief Converts a output_port_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(output_port_info&& obj)
	{
		jopp::object ret;
		ret.insert("provides", to_jopp_object(std::move(obj.provides)));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a output_port_info
	 */
	inline output_port_info make_output_port_info(jopp::object&& obj)
	{
		return output_port_info{
			.provides = make_port_capability(std::move(obj.get_field_as<jopp::object>("provides")))
		};
	}

	/**
	 * \brief A mapping from a port name to a output_port_info
	 */
	using output_port_info_map = std::map<std::string, output_port_info>;

	/**
	 * \brief Converts a output_port_info_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(output_port_info_map&& obj)
	{
		jopp::object ret;
		for(auto&& item : std::move(obj))
		{ ret.insert(jopp::string{item.first}, to_jopp_object(std::move(item.second))); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a output_port_info_map
	 */
	inline output_port_info_map make_output_port_info_map(jopp::object&& obj)
	{
		output_port_info_map ret;
		for(auto&& item: std::move(obj))
		{
			ret.insert(
				std::pair{
					item.first,
					make_output_port_info(std::move(item.second.get<jopp::object>()))
				}
			);
		}
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
	inline jopp::object to_jopp_object(worker_application_info&& obj)
	{
		jopp::object ret;
		ret.insert("display_name", std::move(obj.display_name));
		ret.insert("inputs", to_jopp_object(std::move(obj.inputs)));
		ret.insert("outputs", to_jopp_object(std::move(obj.outputs)));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a worker_application_info
	 */
	inline worker_application_info make_worker_application_info(jopp::object&& obj)
	{
		return worker_application_info{
			.display_name = std::move(obj.get_field_as<std::string>("display_name")),
			.inputs = make_input_port_info_map(std::move(obj.get_field_as<jopp::object>("inputs"))),
			.outputs = make_output_port_info_map(std::move(obj.get_field_as<jopp::object>("output")))
		};
	}

	struct get_worker_application_info
	{};
}

#endif