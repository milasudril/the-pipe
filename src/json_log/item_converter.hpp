#ifndef PIPE_JSON_LOG_OBJECT_CONVERTER_HPP
#define PIPE_JSON_LOG_OBJECT_CONVERTER_HPP

#include "src/log/log.hpp"

#include <jopp/types.hpp>
#include <expected>

namespace Pipe::json_log
{
	/**
	 * \brief Converts item into a jopp::object
	 */
	inline jopp::object to_jopp_object(log::item const& item)
	{
		jopp::object ret;
		ret.insert(
			"when",
			std::chrono::duration<double>(item.when.time_since_epoch()).count()
		);
		ret.insert("message", item.message);
		ret.insert("level", to_string(item.level));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object into an item
	 * \note If conversion fails, a message wrapped in an std::unexpected is returned
	 * \note If the level conveyed by obj is unknown, it is mapped to log::item::level::info
	 */
	inline std::expected<log::item, char const*> make_log_item(jopp::object const& obj)
	{
		auto const when = obj.try_get_field_as<double>("when");
		if(when == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `when` from received log item"}; }

		auto const level = obj.try_get_field_as<std::string>("level");
		if(level == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `level` from received log item"}; }

		auto const message = obj.try_get_field_as<std::string>("message");
		if(message == nullptr)
		{ return std::unexpected{"Failed to extract mandatory field `level` from received log item"}; }

		return log::item{
			.when = log::clock::time_point{}
				+ duration_cast<log::clock::duration>(std::chrono::duration<double>{*when}),
			.level = log::make_level_with_fallback(*level, log::item::level::info),
			.message = std::move(*message)
		};
	}
}

#endif