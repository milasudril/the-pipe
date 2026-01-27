#ifndef PIPE_JSON_LOG_OBJECT_CONVERTER_HPP
#define PIPE_JSON_LOG_OBJECT_CONVERTER_HPP

#include "src/log/log.hpp"

#include <jopp/types.hpp>

namespace Pipe::json_log
{
	inline jopp::object to_jopp_object(log::item const& item)
	{
		jopp::object ret;
		ret.insert(
			"when",
			std::chrono::duration<double>(item.when.time_since_epoch()).count()
		);
		ret.insert("message", item.message);
		ret.insert("severity", to_string(item.severity));
		return ret;
	}

	/**
	 * \todo Actually belongs in a json_log_reader module
	 */
	inline log::item make_log_item(jopp::object const& obj)
	{
		return log::item{
			.when = log::clock::time_point{}
				+ duration_cast<log::clock::duration>(std::chrono::duration<double>{obj.get_field_as<double>("when")}),
			.severity = log::make_severity(obj.get_field_as<std::string>("severity")),
			.message = obj.get_field_as<std::string>("message")
		};
	}
}

#endif