#ifndef PIPE_JSON_LOG_WRITER_WRITER_HPP
#define PIPE_JSON_LOG_WRITER_WRITER_HPP

#include "src/log/log.hpp"
#include <jopp/serializer.hpp>

namespace Pipe::json_log_writer
{
	class writer
	{
	public:
		void write(log::item&& item);

	private:
	};
}

#endif