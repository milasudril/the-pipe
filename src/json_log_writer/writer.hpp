#ifndef PIPE_JSON_LOG_WRITER_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_WRITER_HPP

#include "src/log/log.hpp"
#include <jopp/serializer.hpp>

namespace Pipe::json_log_writer
{
	jopp::object to_jopp_object(log::item const& item)
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

	class writer
	{
	public:
		void write(log::item&& item);

	private:
	};
}

#endif