//@	{"target":{"name":"./disconnection.test"}}

#include "./disconnection.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct dummy
	{};
}

TESTCASE(Pipe_worker_ctl_disconnection_to_jopp_object)
{
	auto const res = to_jopp_object(
		Pipe::worker_ctl::disconnection<dummy>{
			.portname = "foobar"
		}
	);

	EXPECT_EQ(res.get_field_as<jopp::string>("portname"), "foobar");
}

TESTCASE(Pipe_worker_ctl_make_disconnection)
{
	jopp::object obj;
	obj.insert("portname", "foobar");

	auto const res = Pipe::worker_ctl::make_disconnection<dummy>(std::move(obj));
	EXPECT_EQ(res.portname, "foobar");
}

TESTCASE(Pipe_worker_ctl_make_input_disconnection)
{
	jopp::object obj;
	obj.insert("portname", "foobar");

	auto const res = Pipe::worker_ctl::make_input_disconnection(std::move(obj));
	EXPECT_EQ(res.portname, "foobar");
}

TESTCASE(Pipe_worker_ctl_make_output_disconnection)
{
	jopp::object obj;
	obj.insert("portname", "foobar");

	auto const res = Pipe::worker_ctl::make_output_disconnection(std::move(obj));
	EXPECT_EQ(res.portname, "foobar");
}
