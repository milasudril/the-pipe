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
		std::string schema;
		jopp::value format_descriptor;
	};

	/**
	 * \brief Converts a data_format_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(data_format_info&& obj)
	{
		jopp::object ret;
		ret.insert("schema", std::move(obj.schema));
		ret.insert("format_descriptor", std::move(obj.format_descriptor));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a data_format_info
	 */
	inline data_format_info make_data_format_info(jopp::object&& obj)
	{
		auto format_descriptor = obj.find("format_descriptor");
		if(format_descriptor == std::end(obj))
		{ throw std::runtime_error{"Missing mandatory field `format_descriptor`"}; }
		return data_format_info{
			.schema = std::move(obj.get_field_as<std::string>("schema")),
			.format_descriptor = std::move(format_descriptor->second)
		};
	}
}
#endif