//@	{"target":{"name":"connection.test"}}

#include "./connection.hpp"
#include "testfwk/validation.hpp"

#include <jopp/types.hpp>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_ctl_endpoint_path_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::endpoint_path{
			.type = "fs_entry",
			.value = jopp::value{"/foo/bar"}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("type"), "fs_entry");
	EXPECT_EQ(obj.get_field_as<jopp::string>("value"), "/foo/bar");
}

TESTCASE(Pipe_worker_ctl_make_endpoint_path)
{
	jopp::object obj;
	obj.insert("type", "fs_entry");
	obj.insert("value", "/foo/bar");

	auto const endpoint = Pipe::worker_ctl::make_endpoint_path(std::move(obj));
	EXPECT_EQ(endpoint.type, "fs_entry");
	EXPECT_EQ(endpoint.value.get<jopp::string>(), "/foo/bar");
}