//@	{"target":{"name":"file_access_permission.test"}}

#include "./file_access_permission.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_os_services_fs_file_access_permission_bit_to_string)
{
	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::owner_read),
		std::string_view{"owner_read"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::owner_write),
		std::string_view{"owner_write"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::owner_execute),
		std::string_view{"owner_execute"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::group_read),
		std::string_view{"group_read"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::group_write),
		std::string_view{"group_write"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::group_execute),
		std::string_view{"group_execute"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::other_read),
		std::string_view{"other_read"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::other_write),
		std::string_view{"other_write"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_access_permission_bit::other_execute),
		std::string_view{"other_execute"}
	);
}

TESTCASE(Pipe_os_services_fs_make_file_access_permission_bit)
{
	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("owner_read"),
		Pipe::os_services::fs::file_access_permission_bit::owner_read
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("owner_write"),
		Pipe::os_services::fs::file_access_permission_bit::owner_write
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("owner_execute"),
		Pipe::os_services::fs::file_access_permission_bit::owner_execute
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("group_read"),
		Pipe::os_services::fs::file_access_permission_bit::group_read
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("group_write"),
		Pipe::os_services::fs::file_access_permission_bit::group_write
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("group_execute"),
		Pipe::os_services::fs::file_access_permission_bit::group_execute
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("other_read"),
		Pipe::os_services::fs::file_access_permission_bit::other_read
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("other_write"),
		Pipe::os_services::fs::file_access_permission_bit::other_write
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_access_permission_bit("other_execute"),
		Pipe::os_services::fs::file_access_permission_bit::other_execute
	);

	try
	{
		std::ignore = Pipe::os_services::fs::make_file_access_permission_bit("blah");
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Unknown file permission bit"});
	}
}

TESTCASE(Pipe_os_services_fs_file_access_permission_value)
{
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::other_execute), 0001);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::other_write), 0002);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::other_read), 0004);

	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::group_execute), 0010);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::group_write), 0020);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::group_read), 0040);

	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::owner_execute), 0100);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::owner_write), 0200);
	EXPECT_EQ(static_cast<uint16_t>(Pipe::os_services::fs::file_access_permission::owner_read), 0400);
}

TESTCASE(Pipe_os_services_fs_file_access_permission_to_array_of_strings)
{
	{
		auto const result = to_array_of_strings(
			Pipe::os_services::fs::file_access_permission::owner_read|
			Pipe::os_services::fs::file_access_permission::owner_write
		);

		REQUIRE_EQ(std::size(result), 2);
		EXPECT_EQ(result[0], std::string_view{"owner_read"});
		EXPECT_EQ(result[1], std::string_view{"owner_write"});
	}

	{
		auto const result = to_array_of_strings(
			Pipe::os_services::fs::file_access_permission::other_execute
		);

		REQUIRE_EQ(std::size(result), 1);
		EXPECT_EQ(result[0], std::string_view{"other_execute"});
	}
}