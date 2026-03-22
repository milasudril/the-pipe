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

TESTCASE(Pipe_worker_ctl_input_connection_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::input_connection{
			.remote_stream_info = Pipe::worker_ctl::data_stream_info{
				.format = Pipe::worker_ctl::data_format_info{
					.type = "mime",
					.value = jopp::value{"text/plain"}
				},
				.transport_params = Pipe::worker_ctl::data_transport_info{
					.type = "foobar",
					.value = jopp::value{"kaka"}
				}
			},
			.local_portname = "foobar"
		}
	);

	EXPECT_EQ(obj.get_field_as<jopp::string>("local_portname"), "foobar");
	auto const& remote_stream_info = obj.get_field_as<jopp::object>("remote_stream_info");
	auto const& address = remote_stream_info.get_field_as<jopp::object>("transport_params");
	EXPECT_EQ(address.get_field_as<jopp::string>("type"), "foobar");
	EXPECT_EQ(address.get_field_as<jopp::string>("value"), "kaka");
	auto const& format = remote_stream_info.get_field_as<jopp::object>("format");
	EXPECT_EQ(format.get_field_as<jopp::string>("type"), "mime");
	EXPECT_EQ(format.get_field_as<jopp::string>("value"), "text/plain");
}

TESTCASE(Pipe_worker_ctl_make_input_connection)
{
	jopp::object obj_in;
	obj_in.insert("local_portname", "foobar");

	jopp::object remote_stream_info;
	jopp::object transport_params;
	transport_params.insert("type", "fs_entry");
	transport_params.insert("value", "/foo/bar");
	remote_stream_info.insert("transport_params", std::move(transport_params));
	jopp::object format;
	format.insert("type", "mime");
	format.insert("value", "text/plain");
	remote_stream_info.insert("format", std::move(format));
	obj_in.insert("remote_stream_info", std::move(remote_stream_info));

	auto const result = Pipe::worker_ctl::make_input_connection(std::move(obj_in));
	EXPECT_EQ(result.local_portname, "foobar");
	EXPECT_EQ(result.remote_stream_info.transport_params.type, "fs_entry");
	EXPECT_EQ(result.remote_stream_info.transport_params.value.get<jopp::string>(), "/foo/bar");
	EXPECT_EQ(result.remote_stream_info.format.type, "mime");
	EXPECT_EQ(result.remote_stream_info.format.value.get<jopp::string>(), "text/plain");
}

TESTCASE(Pipe_worker_ctl_remote_input_endpoint_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::remote_input_endpoint{
			.address = Pipe::worker_ctl::endpoint_path{
				.type = "fs_entry",
				.value = jopp::value{"/foo/bar"}
			}
		}
	);

	auto const& address = obj.get_field_as<jopp::object>("address");
	EXPECT_EQ(address.get_field_as<jopp::string>("type"), "fs_entry");
	EXPECT_EQ(address.get_field_as<jopp::string>("value"), "/foo/bar");
}

TESTCASE(Pipe_worker_ctl_make_remote_input_endpoint)
{
	jopp::object obj_in;
	jopp::object address;
	address.insert("type", "fs_entry");
	address.insert("value", "/foo/bar");
	obj_in.insert("address", std::move(address));

	auto const remote_endpoint = Pipe::worker_ctl::make_remote_input_endpoint(std::move(obj_in));
	EXPECT_EQ(remote_endpoint.address.type, "fs_entry");
	EXPECT_EQ(remote_endpoint.address.value.get<jopp::string>(), "/foo/bar");
}

TESTCASE(Pipe_worker_ctl_endpoint_open_opts_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::endpoint_open_opts{
			.precond = Pipe::os_services::fs::file_open_precondition::must_exist,
			.created_endpoint_perms = Pipe::os_services::fs::file_access_permission::owner_read
		}
	);

	auto const& created_endpoint_perms = obj.get_field_as<jopp::array>("created_endpoint_perms");
	REQUIRE_EQ(created_endpoint_perms.size(), 1);
	EXPECT_EQ(created_endpoint_perms.begin()->get<jopp::string>(), "owner_read");
}

TESTCASE(Pipe_worker_ctl_make_endpoint_open_opts_default_values)
{
	jopp::object obj_in;

	auto const endpoint_open_opts = Pipe::worker_ctl::make_endpoint_open_opts(obj_in);
	EXPECT_EQ(
		endpoint_open_opts.created_endpoint_perms,
		  Pipe::os_services::fs::file_access_permission::owner_read
		| Pipe::os_services::fs::file_access_permission::owner_write
	);
	EXPECT_EQ(
		endpoint_open_opts.precond,
		Pipe::os_services::fs::file_open_precondition::none
	);
}

TESTCASE(Pipe_worker_ctl_make_endpoint_open_opts)
{
	jopp::object obj_in;
	jopp::array perms;
	perms.push_back("owner_read");
	obj_in.insert("created_endpoint_perms", std::move(perms));
	obj_in.insert("precond", "must_exist");

	auto const endpoint_open_opts = Pipe::worker_ctl::make_endpoint_open_opts(obj_in);
	EXPECT_EQ(
		endpoint_open_opts.created_endpoint_perms,
		  Pipe::os_services::fs::file_access_permission::owner_read
	);
	EXPECT_EQ(
		endpoint_open_opts.precond,
		Pipe::os_services::fs::file_open_precondition::must_exist
	);
}

TESTCASE(Pipe_worker_ctl_output_connection_to_jopp_object)
{
	auto const obj = to_jopp_object(
		Pipe::worker_ctl::output_connection{
			.remote_endpoint = Pipe::worker_ctl::remote_input_endpoint{
				.address = Pipe::worker_ctl::endpoint_path {
					.type = "fs_entry",
					.value = jopp::value{"/foo/bar"}
				}
			},
			.local_portname = "foobar",
			.remote_endpoint_open_opts = Pipe::worker_ctl::endpoint_open_opts{
				.precond = Pipe::os_services::fs::file_open_precondition::must_exist,
				.created_endpoint_perms = Pipe::os_services::fs::file_access_permission::owner_read
			}
		}
	);

	auto const& remote_endpoint = obj.get_field_as<jopp::object>("remote_endpoint");
	auto const& address = remote_endpoint.get_field_as<jopp::object>("address");
	EXPECT_EQ(address.get_field_as<jopp::string>("type"), "fs_entry");
	EXPECT_EQ(address.get_field_as<jopp::string>("value"), "/foo/bar");
	EXPECT_EQ(obj.get_field_as<jopp::string>("local_portname"), "foobar");
	auto const& endpoint_open_opts = obj.get_field_as<jopp::object>("remote_endpoint_open_opts");
	EXPECT_EQ(endpoint_open_opts.get_field_as<jopp::string>("precond"), "must_exist");
	auto const& perms = endpoint_open_opts.get_field_as<jopp::array>("created_endpoint_perms");
	REQUIRE_EQ(perms.size(), 1);
	EXPECT_EQ(perms.begin()->get<jopp::string>(), "owner_read");
}

TESTCASE(Pipe_worker_ctl_make_output_connection_no_open_opts)
{
	jopp::object obj_in;

	obj_in.insert("local_portname", "foobar");

	jopp::object remote_endpoint;
	jopp::object address;
	address.insert("type", "fs_entry");
	address.insert("value", "/foo/bar");
	remote_endpoint.insert("address", std::move(address));
	obj_in.insert("remote_endpoint", std::move(remote_endpoint));

	auto const output_connection = Pipe::worker_ctl::make_output_connection(std::move(obj_in));
	EXPECT_EQ(output_connection.local_portname, "foobar");
	EXPECT_EQ(output_connection.remote_endpoint.address.type, "fs_entry");
	EXPECT_EQ(output_connection.remote_endpoint.address.value.get<jopp::string>(), "/foo/bar");
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
	jopp::object obj_in;

	obj_in.insert("local_portname", "foobar");

	jopp::object remote_endpoint;
	jopp::object address;
	address.insert("type", "fs_entry");
	address.insert("value", "/foo/bar");
	remote_endpoint.insert("address", std::move(address));
	obj_in.insert("remote_endpoint", std::move(remote_endpoint));

	jopp::object remote_endpoint_open_opts;
	jopp::array perms;
	perms.push_back("owner_read");
	remote_endpoint_open_opts.insert("created_endpoint_perms", std::move(perms));
	remote_endpoint_open_opts.insert("precond", "must_exist");
	obj_in.insert("remote_endpoint_open_opts", std::move(remote_endpoint_open_opts));

	auto const output_connection = Pipe::worker_ctl::make_output_connection(std::move(obj_in));
	EXPECT_EQ(output_connection.local_portname, "foobar");
	EXPECT_EQ(output_connection.remote_endpoint.address.type, "fs_entry");
	EXPECT_EQ(output_connection.remote_endpoint.address.value.get<jopp::string>(), "/foo/bar");
	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.created_endpoint_perms,
			Pipe::os_services::fs::file_access_permission::owner_read
	);

	EXPECT_EQ(
		output_connection.remote_endpoint_open_opts.precond,
		Pipe::os_services::fs::file_open_precondition::must_exist
	);
}