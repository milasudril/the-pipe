//@	{"target":{"name":"file_open_precondition.test"}}

#include "./file_open_precondition.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_os_services_fs_file_open_precondition_to_string)
{
	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_open_precondition::must_exist),
		std::string_view{"must_exist"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_open_precondition::none),
		std::string_view{"none"}
	);

	EXPECT_EQ(
		to_string(Pipe::os_services::fs::file_open_precondition::must_not_exist),
		std::string_view{"must_not_exist"}
	);
}

TESTCASE(Pipe_os_services_fs_make_file_open_precondition)
{
	EXPECT_EQ(
		Pipe::os_services::fs::make_file_open_precondition("must_exist"),
		Pipe::os_services::fs::file_open_precondition::must_exist
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_open_precondition("none"),
		Pipe::os_services::fs::file_open_precondition::none
	);

	EXPECT_EQ(
		Pipe::os_services::fs::make_file_open_precondition("must_not_exist"),
		Pipe::os_services::fs::file_open_precondition::must_not_exist
	);

	try
	{
		std::ignore = Pipe::os_services::fs::make_file_open_precondition("asdf");
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Unknown file open precondition"});
	}
}