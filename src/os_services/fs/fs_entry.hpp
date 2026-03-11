#ifndef PIPE_OS_SERVICES_FS_FS_ENTRY_HPP
#define PIPE_OS_SERVICES_FS_FS_ENTRY_HPP

#include "src/utils/utils.hpp"
#include <stdexcept>

namespace Pipe::os_services::fs
{
	enum class open_precondition{must_not_exist = -1, none = 0, must_exist = 1};

	constexpr char const* to_string(open_precondition cond)
	{
		switch(cond)
		{
			case open_precondition::must_not_exist:
				return "must_not_exist";
			case open_precondition::none:
				return "none";
			case open_precondition::must_exist:
				return "must_exist";
			default:
				throw std::range_error{""};
		}
	}

	constexpr open_precondition make_open_precondition(std::string_view str)
	{
		if(str == "must_not_exist")
		{ return open_precondition::must_not_exist; }
		if(str == "none")
		{ return open_precondition::none; }
		if(str == "must_exist")
		{ return open_precondition::must_exist; }

		throw std::runtime_error{"Unknown opening precondition"};
	}

	enum class file_permission {
		other_execute = 0001,
		other_write   = 0002,
		other_read    = 0004,
		group_execute = 0010,
		group_write   = 0020,
		group_read    = 0040,
		owner_execute = 0100,
		owner_write   = 0200,
		owner_read    = 0400
	};

	consteval void enable_bitmask_operators(file_permission){}
}

#endif