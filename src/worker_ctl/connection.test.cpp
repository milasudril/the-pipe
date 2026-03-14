//@	{"target":{"name":"connection.test"}}

#include "./connection.hpp"

#include "testfwk/testsuite.hpp"
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

TESTCASE(Pipe_worker_ctl_remote_output_endpoint_to_jopp_object)
{
	auto const obj = Pipe::worker_ctl::to_jopp_object(
		Pipe::worker_ctl::remote_output_endpoint{
			.endpoint = Pipe::worker_ctl::endpoint_path{
				.type = "fs_entry",
				.value = jopp::value{"/foo/bar"}
			},
			.provides = Pipe::worker_ctl::data_stream_info{
				.format = Pipe::worker_ctl::data_format_info{
					.type = "mime",
					.value = jopp::value{"text/plain"}
				},
				.framing = Pipe::worker_ctl::data_framing_info{
					.type = "dynamic_frame_size",
					.value = jopp::value{}
				}
			}
		}
	);

	auto const& endpoint = obj.get_field_as<jopp::object>("endpoint");
	EXPECT_EQ(endpoint.get_field_as<jopp::string>("type"), "fs_entry");
	EXPECT_EQ(endpoint.get_field_as<jopp::string>("value"), "/foo/bar");

	auto const& provides = obj.get_field_as<jopp::object>("provides");

	auto const& format = provides.get_field_as<jopp::object>("format");
	EXPECT_EQ(format.get_field_as<jopp::string>("type"), "mime");
	EXPECT_EQ(format.get_field_as<jopp::string>("value"), "text/plain");

	auto const& framing = provides.get_field_as<jopp::object>("framing");
	EXPECT_EQ(framing.get_field_as<jopp::string>("type"), "dynamic_frame_size");
	EXPECT_EQ(framing.get_field_as<jopp::null>("value"), jopp::null{});

}

TESTCASE(Pipe_worker_ctl_make_remote_output_endpoint)
{
	jopp::object obj;

	jopp::object endpoint;
	endpoint.insert("type", "fs_entry");
	endpoint.insert("value", "/foo/bar");
	obj.insert("endpoint", std::move(endpoint));

	jopp::object provides;

	jopp::object format;
	format.insert("type", "mime");
	format.insert("value", "text/plain");
	provides.insert("format", std::move(format));

	jopp::object framing;
	framing.insert("type", "dynamic_frame_size");
	framing.insert("value", jopp::null{});
	provides.insert("framing", std::move(framing));

	obj.insert("provides", std::move(provides));

	auto const remote_output = Pipe::worker_ctl::make_remote_output_endpoint(std::move(obj));
	EXPECT_EQ(remote_output.endpoint.type, "fs_entry");
	EXPECT_EQ(remote_output.endpoint.value.get<jopp::string>(), "/foo/bar");
	EXPECT_EQ(remote_output.provides.format.type, "mime");
	EXPECT_EQ(remote_output.provides.format.value.get<jopp::string>(), "text/plain");
	EXPECT_EQ(remote_output.provides.framing.type, "dynamic_frame_size");
	EXPECT_EQ(remote_output.provides.framing.value.get<jopp::null>(), jopp::null{});
}