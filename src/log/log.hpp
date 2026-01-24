#ifndef PIPE_LOG_HPP
#define PIPE_LOG_HPP

#include "src/os_services/io/io.hpp"

#include <format>
#include <string_view>

namespace Pipe::log
{
	enum class severity{info, warning, error};

	void write_message(enum severity severity, std::string_view formatted_message) noexcept;

	template<class ... Args>
	void write_message(enum severity severity, std::format_string<Args...> fmt, Args&&... args) noexcept
	{
		try
		{
			write_message(severity, std::format(fmt, std::format<Args>(args)...));
		}
		catch(...)
		{ abort(); }
	}

	void configure(os_services::io::output_file_descriptor);
};

#endif