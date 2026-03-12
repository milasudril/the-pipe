#ifndef PIPE_OS_SERVICES_FS_FILE_ACCESS_PERMISSION_HPP
#define PIPE_OS_SERVICES_FS_FILE_ACCESS_PERMISSION_HPP

#include "src/utils/utils.hpp"

#include <stdexcept>
#include <span>
#include <vector>
#include <string_view>
#include <cstdint>

namespace Pipe::os_services::fs
{
	enum class file_access_permission_bit:uint16_t
	{
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

	constexpr char const* to_string(file_access_permission_bit permbit)
	{
		switch(permbit)
		{
			case file_access_permission_bit::other_execute:
				return "other_execute";
			case file_access_permission_bit::other_write:
				return "other_write";
			case file_access_permission_bit::other_read:
				return "other_read";
			case file_access_permission_bit::group_execute:
				return "group_execute";
			case file_access_permission_bit::group_write:
				return "group_write";
			case file_access_permission_bit::group_read:
				return "group_read";
			case file_access_permission_bit::owner_execute:
				return "owner_execute";
			case file_access_permission_bit::owner_write:
				return "owner_write";
			case file_access_permission_bit::owner_read:
				return "owner_read";
			default:
				throw std::range_error{""};
		}
	}

	constexpr file_access_permission_bit make_file_access_permission_bit(std::string_view str)
	{
		if(str == "other_execute")
		{ return file_access_permission_bit::other_execute; }
		if(str == "other_write")
		{ return file_access_permission_bit::other_write; }
		if(str == "other_read")
		{ return file_access_permission_bit::other_read; }
		if(str == "group_execute")
		{ return file_access_permission_bit::group_execute; }
		if(str == "group_write")
		{ return file_access_permission_bit::group_write; }
		if(str == "group_read")
		{ return file_access_permission_bit::group_read; }
		if(str == "owner_execute")
		{ return file_access_permission_bit::owner_execute; }
		if(str == "owner_write")
		{ return file_access_permission_bit::owner_write; }
		if(str == "owner_read")
		{ return file_access_permission_bit::owner_read; }

		throw std::runtime_error{"Unknown file permission bit"};
	}

	enum class file_access_permission:uint16_t
	{
		other_execute = 1 << static_cast<uint16_t>(file_access_permission_bit::other_execute),
		other_write   = 1 << static_cast<uint16_t>(file_access_permission_bit::other_write  ),
		other_read    = 1 << static_cast<uint16_t>(file_access_permission_bit::other_read   ),
		group_execute = 1 << static_cast<uint16_t>(file_access_permission_bit::group_execute),
		group_write   = 1 << static_cast<uint16_t>(file_access_permission_bit::group_write  ),
		group_read    = 1 << static_cast<uint16_t>(file_access_permission_bit::group_read   ),
		owner_execute = 1 << static_cast<uint16_t>(file_access_permission_bit::owner_execute),
		owner_write   = 1 << static_cast<uint16_t>(file_access_permission_bit::owner_write  ),
		owner_read    = 1 << static_cast<uint16_t>(file_access_permission_bit::owner_read   )
	};

	consteval void enable_bitmask_operators(file_access_permission){}

	inline std::vector<char const*> to_array_of_strings(file_access_permission perm)
	{
		std::vector<char const*> ret;
		ret.reserve(9);

		uint16_t mask = 0400;
		uint16_t k = 8;
		while(mask != 0)
		{
			if(is_set(perm, file_access_permission{mask}))
			{ ret.push_back(to_string(file_access_permission_bit{k})); }
			--k;
			mask >>= 1;
		}

		return ret;
	}

	template<class StringLike>
	inline file_access_permission make_file_access_permission(
		std::span<StringLike const> strings
	)
	{
		file_access_permission ret{};
		for(auto const& item : strings)
		{
			auto const bit = make_file_access_permission_bit(item);
			ret |= file_access_permission{1 << static_cast<uint16_t>(bit)};
		}

		return ret;
	}
}
#endif