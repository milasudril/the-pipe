//@	{"target":{"name": "data_stream_info.test"}}

#include "./data_stream_info.hpp"
#include "src/worker_ctl/any.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <jopp/types.hpp>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_ctl_data_format_info_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::data_format_info{
			.type = Pipe::worker_ctl::type_descriptor{
				.ns_path = std::vector<std::string>{"foo", "bar"},
				.name = "kaka"
			},
			.value = jopp::value{"mjau"}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("value"), "mjau");
	auto const& type = obj.get_field_as<jopp::object>("type");
	EXPECT_EQ(type.get_field_as<jopp::string>("name"), "kaka");
	auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
	REQUIRE_EQ(ns_path.size(), 2);
	EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo");
	EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar");
}

TESTCASE(Pipe_worker_ctl_make_data_format_info)
{
	jopp::object obj;
	obj.insert("value", "mjau");
	jopp::object type;
	jopp::array ns_path;
	ns_path.push_back("foo");
	ns_path.push_back("bar");
	type.insert("ns_path", std::move(ns_path));
	type.insert("name", "kaka");
	obj.insert("type", std::move(type));

	auto const data_format_info = Pipe::worker_ctl::make_data_format_info(std::move(obj));
	EXPECT_EQ(data_format_info.value.get<jopp::string>(), "mjau");
	EXPECT_EQ(data_format_info.type.name, "kaka");
	EXPECT_EQ(data_format_info.type.ns_path, (std::vector<std::string>{"foo", "bar"}));
}

TESTCASE(Pipe_worker_ctl_data_transport_info_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::data_transport_info{
			.type = Pipe::worker_ctl::type_descriptor{
				.ns_path = std::vector<std::string>{"foo", "bar"},
				.name = "kaka"
			},
			.value = jopp::value{"mjau"}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("value"), "mjau");
	auto const& type = obj.get_field_as<jopp::object>("type");
	EXPECT_EQ(type.get_field_as<jopp::string>("name"), "kaka");
	auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
	REQUIRE_EQ(ns_path.size(), 2);
	EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo");
	EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar");
}

TESTCASE(Pipe_worker_ctl_make_data_transport_info)
{
	jopp::object obj;
	obj.insert("value", "mjau");
	jopp::object type;
	jopp::array ns_path;
	ns_path.push_back("foo");
	ns_path.push_back("bar");
	type.insert("ns_path", std::move(ns_path));
	type.insert("name", "kaka");
	obj.insert("type", std::move(type));

	auto const data_format_info = Pipe::worker_ctl::make_data_transport_info(std::move(obj));
	EXPECT_EQ(data_format_info.value.get<jopp::string>(), "mjau");
	EXPECT_EQ(data_format_info.type.name, "kaka");
	EXPECT_EQ(data_format_info.type.ns_path, (std::vector<std::string>{"foo", "bar"}));
}

TESTCASE(Pipe_worker_ctl_data_stream_info_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::data_stream_info{
			.format = Pipe::worker_ctl::data_format_info{
				.type = Pipe::worker_ctl::type_descriptor{
					.ns_path = std::vector<std::string>{"foo", "bar"},
					.name = "kaka"
				},
				.value = jopp::value{"mjau"}
			},
			.transport_params = Pipe::worker_ctl::data_transport_info{
				.type = Pipe::worker_ctl::type_descriptor{
					.ns_path = std::vector<std::string>{"foo3", "bar8"},
					.name = "bulle"
				},
				.value = jopp::value{"vov"}
			}
		}
	);

	{
		auto const& format = obj.get_field_as<jopp::object>("format");
		EXPECT_EQ(format.get_field_as<jopp::string>("value"), "mjau");
		auto const& type = format.get_field_as<jopp::object>("type");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "kaka");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 2);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo");
		EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar");
	}

	{
		auto const& transport_params = obj.get_field_as<jopp::object>("transport_params");
		EXPECT_EQ(transport_params.get_field_as<jopp::string>("value"), "vov");
		auto const& type = transport_params.get_field_as<jopp::object>("type");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "bulle");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 2);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo3");
		EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar8");
	}
}

TESTCASE(Pipe_worker_ctl_make_data_stream_info)
{
	jopp::object obj;

	{
		jopp::object format;
		format.insert("value", "mjau");
		jopp::object type;
		jopp::array ns_path;
		ns_path.push_back("foo");
		ns_path.push_back("bar");
		type.insert("ns_path", std::move(ns_path));
		type.insert("name", "kaka");
		format.insert("type", std::move(type));
		obj.insert("format", std::move(format));
	}

	{
		jopp::object transport_params;
		transport_params.insert("value", "vov");
		jopp::object type;
		jopp::array ns_path;
		ns_path.push_back("foo3");
		ns_path.push_back("bar8");
		type.insert("ns_path", std::move(ns_path));
		type.insert("name", "bulle");
		transport_params.insert("type", std::move(type));
		obj.insert("transport_params", std::move(transport_params));
	}

	auto const data_stream_info = Pipe::worker_ctl::make_data_stream_info(std::move(obj));

	auto const& data_format_info = data_stream_info.format;
	EXPECT_EQ(data_format_info.value.get<jopp::string>(), "mjau");
	EXPECT_EQ(data_format_info.type.name, "kaka");
	EXPECT_EQ(data_format_info.type.ns_path, (std::vector<std::string>{"foo", "bar"}));

	auto const& data_transport_info = data_stream_info.transport_params;
	EXPECT_EQ(data_transport_info.value.get<jopp::string>(), "vov");
	EXPECT_EQ(data_transport_info.type.name, "bulle");
	EXPECT_EQ(data_transport_info.type.ns_path, (std::vector<std::string>{"foo3", "bar8"}));
}

TESTCASE(Pipe_worker_ctl_port_capability_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::port_capability{
			.format =  Pipe::worker_ctl::data_format_info{
				.type = Pipe::worker_ctl::type_descriptor{
					.ns_path = std::vector<std::string>{"foo", "bar"},
					.name = "kaka"
				},
				.value = jopp::value{"mjau"}
			},
			.transport_method = Pipe::worker_ctl::type_descriptor{
				.ns_path = std::vector<std::string>{"foo1", "bar20"},
				.name = "bulle"
			}
		}
	);

	{
		auto const& format = obj.get_field_as<jopp::object>("format");
		EXPECT_EQ(format.get_field_as<jopp::string>("value"), "mjau");
		auto const& type = format.get_field_as<jopp::object>("type");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "kaka");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 2);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo");
		EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar");
	}

	{
		auto const& transport_method = obj.get_field_as<jopp::object>("transport_method");
		EXPECT_EQ(transport_method.get_field_as<jopp::string>("name"), "bulle");
		auto const& ns_path = transport_method.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 2);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "foo1");
		EXPECT_EQ((1 +ns_path.begin())->get<jopp::string>(), "bar20");
	}
}

TESTCASE(Pipe_worker_ctl_make_port_capability)
{
	jopp::object obj;
	{
		jopp::object format;
		format.insert("value", "mjau");
		jopp::object type;
		jopp::array ns_path;
		ns_path.push_back("foo");
		ns_path.push_back("bar");
		type.insert("ns_path", std::move(ns_path));
		type.insert("name", "kaka");
		format.insert("type", std::move(type));
		obj.insert("format", std::move(format));
	}

	{
		jopp::object transport_method;
		jopp::array ns_path;
		ns_path.push_back("foo3");
		ns_path.push_back("bar8");
		transport_method.insert("ns_path", std::move(ns_path));
		transport_method.insert("name", "bulle");
		obj.insert("transport_method", std::move(transport_method));
	}

	auto const port_capability = Pipe::worker_ctl::make_port_capability(std::move(obj));

	auto const& data_format_info = port_capability.format;
	EXPECT_EQ(data_format_info.value.get<jopp::string>(), "mjau");
	EXPECT_EQ(data_format_info.type.name, "kaka");
	EXPECT_EQ(data_format_info.type.ns_path, (std::vector<std::string>{"foo", "bar"}));

	auto const& transport_method = port_capability.transport_method;
	EXPECT_EQ(transport_method.name, "bulle");
	EXPECT_EQ(transport_method.ns_path, (std::vector<std::string>{"foo3", "bar8"}));
}