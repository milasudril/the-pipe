#ifndef PIPE_JSON_LOG_OBJECT_CONVERTER_HPP
#define PIPE_JSON_LOG_OBJECT_CONVERTER_HPP

#include "src/log/log.hpp"

#include <jopp/types.hpp>
#include <expected>

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

	inline std::expected<log::item, char const*> make_log_item(jopp::object const& obj)
	{
		auto const when = obj.try_get_field_as<double>("when");
		if(when == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `when` from received log item"}; }

		auto const severity = obj.try_get_field_as<std::string>("severity");
		if(severity == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `severity` from received log item"}; }

		auto const message = obj.try_get_field_as<std::string>("message");
		if(message == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `severity` from received log item"}; }

		return log::item{
			.when = log::clock::time_point{}
				+ duration_cast<log::clock::duration>(std::chrono::duration<double>{*when}),
			.severity = log::make_severity_with_fallback(*severity, log::item::severity::info),
			.message = std::move(*message)
		};
	}
}

#endif