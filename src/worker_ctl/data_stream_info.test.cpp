//@	{"target":{"name": "data_stream_info.test"}}

#include "./data_stream_info.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_ctl_make_data_format_info)
{
	jopp::object obj_in;
	obj_in.insert("type", "mime");
	obj_in.insert("value", "text/plain");

	auto obj = Pipe::worker_ctl::make_data_format_info(std::move(obj_in));
	EXPECT_EQ(obj.type, "mime");
	EXPECT_EQ(obj.value.get<jopp::string>(), "text/plain");
}

TESTCASE(Pipe_worker_ctl_make_data_transport_info)
{
	jopp::object obj_in;
	obj_in.insert("type", "foobar");

	auto obj = Pipe::worker_ctl::make_data_transport_info(std::move(obj_in));
	EXPECT_EQ(obj.type.front(), "foobar");
	EXPECT_EQ(obj.type.back(), "baź");
	EXPECT_EQ(obj.value.get<jopp::null>(), jopp::null{});
}

TESTCASE(Pipe_worker_ctl_data_stream_info_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::data_stream_info{
			.format = Pipe::worker_ctl::data_format_info{
				.type = "mime",
				.value = jopp::value{"text/plain"}
			},
			.transport_params = Pipe::worker_ctl::data_transport_info{
				.type = std::vector<std::string>{"foobar", "bulle"},
				.value = jopp::value{}
			}
		}
	);

	auto const& format = obj.get_field_as<jopp::object>("format");
	EXPECT_EQ(format.get_field_as<jopp::string>("type"), "mime");
	EXPECT_EQ(format.get_field_as<jopp::string>("value"), "text/plain");

	auto const& transport_params = obj.get_field_as<jopp::object>("transport_params");
	EXPECT_EQ(transport_params.get_field_as<jopp::string>("type"), "foobar");
	EXPECT_EQ(transport_params.get_field_as<jopp::null>("value"), jopp::null{});
}

TESTCASE(Pipe_worker_ctl_make_data_stream_info)
{
	jopp::object obj_in;

	jopp::object format;
	format.insert("type", "mime");
	format.insert("value", "text/plain");
	obj_in.insert("format", std::move(format));

	jopp::object transport_params;
	transport_params.insert("type", "foobar");
	obj_in.insert("transport_params", std::move(transport_params));

	auto const obj = Pipe::worker_ctl::make_data_stream_info(std::move(obj_in));
	EXPECT_EQ(obj.format.type, "mime");
	EXPECT_EQ(obj.format.value.get<jopp::string>(), "text/plain");
	EXPECT_EQ(obj.transport_params.type.front(), "foobar");
	EXPECT_EQ(obj.transport_params.type.back(), "bulle");
	EXPECT_EQ(obj.transport_params.value.get<jopp::null>(), jopp::null{});
}
