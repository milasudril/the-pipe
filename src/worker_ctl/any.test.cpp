//@	{"target":{"name": "any.test"}}

#include "./any.hpp"
#include "src/worker_ctl/data_stream_info.hpp"

#include <jopp/serializer.hpp>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_ctl_type_descriptor_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::type_descriptor{
			.ns_path = std::vector<std::string>{"builtins", "foobar"},
			.name = "kaka"
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("name"), "kaka");
	auto const& ns_path = obj.get_field_as<jopp::array>("ns_path");
	REQUIRE_EQ(std::size(ns_path), 2);
	EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "builtins");
	EXPECT_EQ((ns_path.begin() + 1)->get<jopp::string>(), "foobar");
}

TESTCASE(Pipe_worker_ctl_make_type_descriptor)
{
	jopp::object obj;
	obj.insert("name", "kaka");
	jopp::array ns_path;
	ns_path.push_back("builtins");
	ns_path.push_back("foobar");
	obj.insert("ns_path", std::move(ns_path));

	auto const type_descriptor = Pipe::worker_ctl::make_type_descriptor(std::move(obj));
	EXPECT_EQ(type_descriptor.name, "kaka");
	EXPECT_EQ(type_descriptor.ns_path, (std::vector<std::string>{"builtins", "foobar"}));
}

namespace
{
	struct dummy{};
}

TESTCASE(Pipe_worker_ctl_any_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::any<dummy>{
			.type = Pipe::worker_ctl::type_descriptor{
				.ns_path = std::vector<std::string>{"builtins", "foobar"},
				.name = "bulle"
			},
			.value = jopp::value{42.0}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::number>("value"), 42.0);
	auto const& type = obj.get_field_as<jopp::object>("type");
	EXPECT_EQ(type.get_field_as<jopp::string>("name"), "bulle");
	auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
	REQUIRE_EQ(std::size(ns_path), 2);
	EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "builtins");
	EXPECT_EQ((ns_path.begin() + 1)->get<jopp::string>(), "foobar");
}

TESTCASE(Pipe_worker_ctl_make_any)
{
	jopp::object obj;
	obj.insert("value", 42.0);

	{
		jopp::object type;
		type.insert("name", "kaka");
		{
			jopp::array ns_path;
			ns_path.push_back("builtins");
			ns_path.push_back("foobar");
			type.insert("ns_path", std::move(ns_path));
		}
		obj.insert("type", std::move(type));
	}

	puts(to_string(obj).c_str());

	auto const any = Pipe::worker_ctl::make_any<dummy>(std::move(obj));
	EXPECT_EQ(any.value.get<jopp::number>(), 42.0);
	EXPECT_EQ(any.type.name, "kaka");
	EXPECT_EQ(any.type.ns_path, (std::vector<std::string>{"builtins", "foobar"}));
}