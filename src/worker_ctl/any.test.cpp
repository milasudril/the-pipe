//@	{"target":{"name": "any.test"}}

#include "./any.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct dummy{};
}

TESTCASE(Pipe_worker_ctl_any_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::any<dummy>{
			.type = "number",
			.value = jopp::value{42.0}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("type"), "number");
	EXPECT_EQ(obj.get_field_as<jopp::number>("value"), 42.0);
}

TESTCASE(Pipe_worker_ctl_make_any_no_value)
{
	jopp::object obj_in;
	obj_in.insert("type", "foo");
	auto obj = Pipe::worker_ctl::make_any<dummy, std::string>(std::move(obj_in));
	EXPECT_EQ(obj.type, "foo");
	EXPECT_EQ(obj.value.get<jopp::null>(), jopp::null{});
}

TESTCASE(Pipe_worker_ctl_make_any_with_value)
{
	jopp::object obj_in;
	obj_in.insert("type", "foo");
	obj_in.insert("value", 42.0);

	auto obj = Pipe::worker_ctl::make_any<dummy, std::string>(std::move(obj_in));
	EXPECT_EQ(obj.type, "foo");
	EXPECT_EQ(obj.value.get<jopp::number>(), 42.0);
}
