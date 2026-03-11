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

	enum class file_permission_bit{
		other_execute = 0,
		other_write   = 1,
		other_read    = 2,
		group_execute = 3,
		group_write   = 4,
		group_read    = 5,
		owner_execute = 6,
		owner_write   = 7,
		owner_read    = 8
	};

	enum class file_permission {
		other_execute = 1<<static_cast<int>(file_permission_bit::other_execute),
		other_write   = 1<<static_cast<int>(file_permission_bit::other_write  ),
		other_read    = 1<<static_cast<int>(file_permission_bit::other_read   ),
		group_execute = 1<<static_cast<int>(file_permission_bit::group_execute),
		group_write   = 1<<static_cast<int>(file_permission_bit::group_write  ),
		group_read    = 1<<static_cast<int>(file_permission_bit::group_read   ),
		owner_execute = 1<<static_cast<int>(file_permission_bit::owner_execute),
		owner_write   = 1<<static_cast<int>(file_permission_bit::owner_write  ),
		owner_read    = 1<<static_cast<int>(file_permission_bit::owner_read   )
	};

	consteval void enable_bitmask_operators(file_permission){}
}

#endif