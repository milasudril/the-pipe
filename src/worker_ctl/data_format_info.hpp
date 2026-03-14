#ifndef PIPE_WORKER_CTL_DATA_FORMAT_INFO_HPP
#define PIPE_WORKER_CTL_DATA_FORMAT_INFO_HPP

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	/**
	 * \brief Contains information about the data format used by ports
	 */
	struct data_format_info
	{
		std::string type;
		jopp::value value;
	};

	/**
	 * \brief Converts a data_format_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(data_format_info&& obj)
	{
		jopp::object ret;
		ret.insert("type", std::move(obj.type));
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a data_format_info
	 */
	inline data_format_info make_data_format_info(jopp::object&& obj)
	{
		auto value = obj.find("value");
		if(value == std::end(obj))
		{ throw std::runtime_error{"Missing mandatory field `value`"}; }
		return data_format_info{
			.type = std::move(obj.get_field_as<std::string>("type")),
			.value = std::move(value->second)
		};
	}
}
#endif