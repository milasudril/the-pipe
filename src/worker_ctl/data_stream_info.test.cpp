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

TESTCASE(Pipe_worker_ctl_make_data_framing_info)
{
	jopp::object obj_in;
	obj_in.insert("type", "dynamic_frame_size");

	auto obj = Pipe::worker_ctl::make_data_framing_info(std::move(obj_in));
	EXPECT_EQ(obj.type, "dynamic_frame_size");
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
			.framing = Pipe::worker_ctl::data_framing_info{
				.type = "dynamic_frame_size",
				.value = jopp::value{}
			}
		}
	);

	auto const& format = obj.get_field_as<jopp::object>("format");
	EXPECT_EQ(format.get_field_as<jopp::string>("type"), "mime");
	EXPECT_EQ(format.get_field_as<jopp::string>("value"), "text/plain");

	auto const& framing = obj.get_field_as<jopp::object>("framing");
	EXPECT_EQ(framing.get_field_as<jopp::string>("type"), "dynamic_frame_size");
	EXPECT_EQ(framing.get_field_as<jopp::null>("value"), jopp::null{});
}

TESTCASE(Pipe_worker_ctl_make_data_stream_info)
{
	jopp::object obj_in;

	jopp::object format;
	format.insert("type", "mime");
	format.insert("value", "text/plain");
	obj_in.insert("format", std::move(format));

	jopp::object framing;
	framing.insert("type", "dynamic_frame_size");
	obj_in.insert("framing", std::move(framing));

	auto const obj = Pipe::worker_ctl::make_data_stream_info(std::move(obj_in));
	EXPECT_EQ(obj.format.type, "mime");
	EXPECT_EQ(obj.format.value.get<jopp::string>(), "text/plain");
	EXPECT_EQ(obj.framing.type, "dynamic_frame_size");
	EXPECT_EQ(obj.framing.value.get<jopp::null>(), jopp::null{});
}
