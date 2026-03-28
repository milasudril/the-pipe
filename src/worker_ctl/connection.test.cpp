//@	{"target":{"name":"connection.test"}}

#include "./connection.hpp"

#include "src/os_services/fs/file_access_permission.hpp"
#include "src/os_services/fs/file_open_precondition.hpp"
#include "src/worker_ctl/any.hpp"
#include "src/worker_ctl/data_stream_info.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <jopp/types.hpp>
#include <jopp/serializer.hpp>

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_ctl_input_connection_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::input_connection{
			.local_portname = "input",
			.remote_stream_info = Pipe::worker_ctl::data_stream_info{
				.format = Pipe::worker_ctl::data_format_info{
					.type = Pipe::worker_ctl::type_descriptor{
						.ns_path = std::vector<std::string>{"builtins"},
						.name = "mime"
					},
					.value = jopp::value{"text/plain"}
				},
				.transport_params = Pipe::worker_ctl::data_transport_info{
					.type = Pipe::worker_ctl::type_descriptor{
						.ns_path = std::vector<std::string>{"testpath"},
						.name = "a_name"
					},
					.value = jopp::value{456.0}
				}
			}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("local_portname"), "input");
	auto const& remote_stream_info = obj.get_field_as<jopp::object>("remote_stream_info");
	{
		auto const& format = remote_stream_info.get_field_as<jopp::object>("format");
		auto const& type = format.get_field_as<jopp::object>("type");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 1);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "builtins");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "mime");
		EXPECT_EQ(format.get_field_as<jopp::string>("value"), "text/plain");
	}

	{
		auto const& transport_params = remote_stream_info.get_field_as<jopp::object>("transport_params");
		auto const& type = transport_params.get_field_as<jopp::object>("type");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 1);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "testpath");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "a_name");
		EXPECT_EQ(transport_params.get_field_as<jopp::number>("value"), 456.0);
	}
}

TESTCASE(Pipe_worker_ctl_make_input_connection)
{
	jopp::object obj;
	obj.insert("local_portname", "input");

	{
		jopp::object remote_stream_info;
		{
			jopp::object format;
			{
				jopp::object type;
				{
					jopp::array ns_path;
					ns_path.push_back("builtins");
					type.insert("ns_path", std::move(ns_path));
					type.insert("name", "mime");
				}
				format.insert("type", std::move(type));
				format.insert("value", "text/plain");
			}
			remote_stream_info.insert("format", std::move(format));
		}

		{
			jopp::object transport_params;
			{
				jopp::object type;
				jopp::array ns_path;
				ns_path.push_back("testpath");
				type.insert("ns_path", std::move(ns_path));
				type.insert("name", "a_name");
				transport_params.insert("type", std::move(type));
			}
			transport_params.insert("value", 456.0);
			remote_stream_info.insert("transport_params", std::move(transport_params));
		}

		obj.insert("remote_stream_info", std::move(remote_stream_info));
	}

	auto const input_connection = Pipe::worker_ctl::make_input_connection(std::move(obj));
	EXPECT_EQ(input_connection.local_portname, "input");
	EXPECT_EQ(input_connection.remote_stream_info.format.type.name, "mime");
	EXPECT_EQ(input_connection.remote_stream_info.format.type.ns_path, std::vector<std::string>{"builtins"});
	EXPECT_EQ(input_connection.remote_stream_info.format.value.get<jopp::string>(), "text/plain");
	EXPECT_EQ(
		input_connection.remote_stream_info.transport_params.type.ns_path,
		std::vector<std::string>{"testpath"}
	);
	EXPECT_EQ(input_connection.remote_stream_info.transport_params.type.name, "a_name");
	EXPECT_EQ(input_connection.remote_stream_info.transport_params.value.get<jopp::number>(), 456.0);
}

TESTCASE(Pipe_worker_ctl_output_connection_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::output_connection{
			.local_portname = "foobar",
			.remote_endpoint = Pipe::worker_ctl::data_transport_info{
				.type = Pipe::worker_ctl::type_descriptor{
					.ns_path = std::vector<std::string>{"level_1", "level_2"},
					.name = "my_type"
				},
				.value = jopp::value{"a_name"}
			},
			.remote_endpoint_open_opts = Pipe::worker_ctl::endpoint_open_opts{
				.precond = Pipe::os_services::fs::file_open_precondition::must_exist,
				.created_endpoint_perms = Pipe::os_services::fs::file_access_permission::owner_read
			}
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("local_portname"), "foobar");
	{
		auto const& remote_endpoint = obj.get_field_as<jopp::object>("remote_endpoint");
		EXPECT_EQ(remote_endpoint.get_field_as<jopp::string>("value"), "a_name");
		auto const& type = remote_endpoint.get_field_as<jopp::object>("type");
		auto const& ns_path = type.get_field_as<jopp::array>("ns_path");
		REQUIRE_EQ(ns_path.size(), 2);
		EXPECT_EQ(ns_path.begin()->get<jopp::string>(), "level_1");
		EXPECT_EQ((ns_path.begin() + 1)->get<jopp::string>(), "level_2");
		EXPECT_EQ(type.get_field_as<jopp::string>("name"), "my_type");
	}

	{
		auto const& remote_endpoint_open_opts = obj.get_field_as<jopp::object>("remote_endpoint_open_opts");
		EXPECT_EQ(remote_endpoint_open_opts.get_field_as<jopp::string>("precond"), "must_exist");

		auto const& created_endpoint_perms =
			remote_endpoint_open_opts.get_field_as<jopp::array>("created_endpoint_perms");
		REQUIRE_EQ(created_endpoint_perms.size(), 1);
		EXPECT_EQ(created_endpoint_perms.begin()->get<jopp::string>(), "owner_read");
	}
}

TESTCASE(Pipe_worker_ctl_make_output_connection_no_open_opts)
{
	jopp::object obj;
	obj.insert("local_portname", "foobar");

	{
		jopp::object remote_endpoint;
		{
			jopp::object type;
			{
				jopp::array ns_path;
				ns_path.push_back("level_1");
				ns_path.push_back("level_2");
				type.insert("ns_path", std::move(ns_path));
				type.insert("name", "my_type");
			}
			remote_endpoint.insert("type", std::move(type));
			remote_endpoint.insert("value", "a_name");
		}
		obj.insert("remote_endpoint", std::move(remote_endpoint));
	}

	auto const output_connection = Pipe::worker_ctl::make_output_connection(std::move(obj));
	EXPECT_EQ(output_connection.local_portname, "foobar");
	EXPECT_EQ(output_connection.remote_endpoint.type.name, "my_type");
	EXPECT_EQ(
		output_connection.remote_endpoint.type.ns_path,
		(std::vector<std::string>{"level_1", "level_2"})
	);
	EXPECT_EQ(output_connection.remote_endpoint.value.get<jopp::string>(), "a_name");
	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.created_endpoint_perms,
				Pipe::os_services::fs::file_access_permission::owner_read
			| Pipe::os_services::fs::file_access_permission::owner_write
	);
	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.precond,
		Pipe::os_services::fs::file_open_precondition::none
	);
}

TESTCASE(Pipe_worker_ctl_make_output_connection)
{
	jopp::object obj;
	obj.insert("local_portname", "foobar");

	{
		jopp::object remote_endpoint;
		{
			jopp::object type;
			{
				jopp::array ns_path;
				ns_path.push_back("level_1");
				ns_path.push_back("level_2");
				type.insert("ns_path", std::move(ns_path));
				type.insert("name", "my_type");
			}
			remote_endpoint.insert("type", std::move(type));
			remote_endpoint.insert("value", "a_name");
		}
		obj.insert("remote_endpoint", std::move(remote_endpoint));
	}
	{
		jopp::object remote_endpoint_open_opts;
		remote_endpoint_open_opts.insert("precond", "must_exist");
		{
			jopp::array created_endpoint_perms;
			created_endpoint_perms.push_back("owner_read");
			remote_endpoint_open_opts.insert("created_endpoint_perms", std::move(created_endpoint_perms));
		}
		obj.insert("remote_endpoint_open_opts", std::move(remote_endpoint_open_opts));
	}

	auto const output_connection = Pipe::worker_ctl::make_output_connection(std::move(obj));
	EXPECT_EQ(output_connection.local_portname, "foobar");
	EXPECT_EQ(output_connection.remote_endpoint.type.name, "my_type");
	EXPECT_EQ(
		output_connection.remote_endpoint.type.ns_path,
		(std::vector<std::string>{"level_1", "level_2"})
	);
	EXPECT_EQ(output_connection.remote_endpoint.value.get<jopp::string>(), "a_name");
	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.created_endpoint_perms,
		Pipe::os_services::fs::file_access_permission::owner_read
	);
	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.precond,
		Pipe::os_services::fs::file_open_precondition::must_exist
	);
}