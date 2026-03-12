//@	{"target":{"name":"file_access_permission.test"}}

#include "./file_access_permission.hpp"

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