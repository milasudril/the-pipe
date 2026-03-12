#ifndef PIPE_OS_SERVICES_FS_FS_ENTRY_HPP
#define PIPE_OS_SERVICES_FS_FS_ENTRY_HPP

#include "src/utils/utils.hpp"

#include <stdexcept>
#include <string_view>

namespace Pipe::os_services::fs
{
	enum class file_open_precondition{must_not_exist = -1, none = 0, must_exist = 1};

	constexpr char const* to_string(file_open_precondition cond)
	{
		switch(cond)
		{
			case file_open_precondition::must_not_exist:
				return "must_not_exist";
			case file_open_precondition::none:
				return "none";
			case file_open_precondition::must_exist:
				return "must_exist";
			default:
				throw std::range_error{""};
		}
	}

	constexpr file_open_precondition make_file_open_precondition(std::string_view str)
	{
		if(str == "must_not_exist")
		{ return file_open_precondition::must_not_exist; }
		if(str == "none")
		{ return file_open_precondition::none; }
		if(str == "must_exist")
		{ return file_open_precondition::must_exist; }

		throw std::runtime_error{"Unknown opening precondition"};
	}
}

#endif