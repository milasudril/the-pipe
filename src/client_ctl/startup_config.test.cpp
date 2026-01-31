//@	{"target": {"name": "startup_config.test"}}

#include "./startup_config.hpp"

#include "src/os_services/ipc/eventfd.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "testfwk/validation.hpp"

#include <jopp/types.hpp>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_client_ctl_startup_config_socket_fd_ref_to_jopp_number)
{
	auto const result = Pipe::client_ctl::to_jopp_value(Pipe::client_ctl::socket_fd_ref{346});
	EXPECT_EQ(result, 346.0);
}

TESTCASE(Pipe_client_ctl_startup_config_make_socket_fd_ref_from_jopp_number_out_of_range)
{
	try
	{
		jopp::number val{-34435.0};
		std::ignore = Pipe::client_ctl::make_socket_fd_ref(val);
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid socket_fd"}); }

	try
	{
		jopp::number val{2147483648.0};
		std::ignore = Pipe::client_ctl::make_socket_fd_ref(val);
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid socket_fd"}); }
}

TESTCASE(Pipe_client_ctl_startup_config_make_socket_fd_ref_from_jopp_number_invalid)
{
	try
	{
		jopp::number val{3456.2};
		std::ignore = Pipe::client_ctl::make_socket_fd_ref(val);
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid socket_fd"}); }
}

TESTCASE(Pipe_client_ctl_startup_config_make_socket_fd_ref_from_jopp_number_bad_fd)
{
	try
	{
		jopp::number val{4965834.0};
		std::ignore = Pipe::client_ctl::make_socket_fd_ref(val);
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid socket_fd: Bad file descriptor"}); }
}

TESTCASE(Pipe_client_ctl_startup_config_make_socket_fd_ref_from_jopp_number_not_a_socket)
{
	try
	{
		auto fd = Pipe::os_services::ipc::make_eventfd();
		jopp::number val{static_cast<double>(fd.get().native_handle())};
		std::ignore = Pipe::client_ctl::make_socket_fd_ref(val);
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"socket_fd is not a socket"}); }
}

TESTCASE(Pipe_client_ctl_startup_config_make_socket_fd_ref_from_jopp_number)
{
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	jopp::number val{static_cast<double>(sockets.socket_a().native_handle())};
	auto const result = Pipe::client_ctl::make_socket_fd_ref(val);
	EXPECT_EQ(result.native_handle(), sockets.socket_a());
}

TESTCASE(Pipe_client_ctl_startup_config_host_address_to_jopp_object)
{
	auto const result = Pipe::client_ctl::to_jopp_object(
		Pipe::client_ctl::socket_fd_ref{346}
	);

	auto const type = result.get_field_as<std::string>("type");
	auto const value = result.get_field_as<double>("value");

	EXPECT_EQ(type, "socket_fd");
	EXPECT_EQ(value, 346.0);
}

TESTCASE(Pipe_client_ctl_startup_config_make_host_address_from_jopp_object_unsupported_type)
{
	try
	{
		jopp::object obj;
		obj.insert("type", "foobar");
		std::ignore = Pipe::client_ctl::make_host_address(obj);
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The given host address type is not supported"});
	}
}

TESTCASE(Pipe_client_ctl_startup_config_make_host_address_from_jopp_object_type_socket_fd)
{
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	jopp::object obj;
	obj.insert("type", "socket_fd");
	obj.insert("value", static_cast<double>(sockets.socket_a().native_handle()));

	auto const result = Pipe::client_ctl::make_host_address(obj);
	EXPECT_EQ(
		std::get<Pipe::client_ctl::socket_fd_ref>(result).native_handle(),
		sockets.socket_a().native_handle()
	);
}

TESTCASE(Pipe_client_ctl_startup_config_host_info_to_jopp_object)
{
	auto const result = Pipe::client_ctl::to_jopp_object(
		Pipe::client_ctl::host_info{
			.address = Pipe::client_ctl::socket_fd_ref{346}
		}
	);

	auto const& address = result.get_field_as<jopp::object>("address");
	auto const& addr_type = address.get_field_as<std::string>("type");
	auto const& addr_value = address.get_field_as<double>("value");
	EXPECT_EQ(addr_type, "socket_fd");
	EXPECT_EQ(addr_value, 346.0);
}

TESTCASE(Pipe_client_ctl_startup_config_make_host_info_from_jopp_object)
{
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	jopp::object address;
	address.insert("type", "socket_fd");
	address.insert("value", static_cast<double>(sockets.socket_a().native_handle()));

	jopp::object obj;
	obj.insert("address", std::move(address));

	auto const result = Pipe::client_ctl::make_host_info(obj);
	EXPECT_EQ(
		std::get<Pipe::client_ctl::socket_fd_ref>(result.address).native_handle(),
		sockets.socket_a().native_handle()
	);
}

TESTCASE(Pipe_client_ctl_startup_config_input_port_file_map_to_jopp_object)
{
	auto const result = Pipe::client_ctl::to_jopp_object(
		Pipe::client_ctl::input_port_file_map{
			{"port_1", "Concrete_Scuffed_Industrial_BaseColor.exr"},
			{"port_2", "Steel_Galvanized_Rusted_Metallic.exr"},
			{"port_3", "Steel_Galvanized_Rusted_Metallic.exr"},
			{"port_4", "Brick_Victorian_Red_Normal.exr"},
			{"port_5", "Asphalt_Wet_Cracked_Roughness.exr"},
			{"port_6", "Corrugated_Metal_Panel_Opacity.exr"}
		}
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_1"),
		"Concrete_Scuffed_Industrial_BaseColor.exr"
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_2"),
		"Steel_Galvanized_Rusted_Metallic.exr"
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_3"),
		"Steel_Galvanized_Rusted_Metallic.exr"
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_4"),
		"Brick_Victorian_Red_Normal.exr"
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_5"),
		"Asphalt_Wet_Cracked_Roughness.exr"
	);

	EXPECT_EQ(
		result.get_field_as<std::string>("port_6"),
		"Corrugated_Metal_Panel_Opacity.exr"
	);
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_input_port_file_map)
{
	jopp::object obj;
	obj.insert("port_1", "Concrete_Scuffed_Industrial_BaseColor.exr");
	obj.insert("port_2", "Steel_Galvanized_Rusted_Metallic.exr");
	obj.insert("port_3", "Steel_Galvanized_Rusted_Metallic.exr");
	obj.insert("port_4", "Brick_Victorian_Red_Normal.exr");
	obj.insert("port_5", "Asphalt_Wet_Cracked_Roughness.exr");
	obj.insert("port_6", "Corrugated_Metal_Panel_Opacity.exr");

	auto const result = Pipe::client_ctl::make_input_port_file_map(obj);

	EXPECT_EQ(result.at("port_1"), "Concrete_Scuffed_Industrial_BaseColor.exr");
	EXPECT_EQ(result.at("port_2"), "Steel_Galvanized_Rusted_Metallic.exr");
	EXPECT_EQ(result.at("port_3"), "Steel_Galvanized_Rusted_Metallic.exr");
	EXPECT_EQ(result.at("port_4"), "Brick_Victorian_Red_Normal.exr");
	EXPECT_EQ(result.at("port_5"), "Asphalt_Wet_Cracked_Roughness.exr");
	EXPECT_EQ(result.at("port_6"), "Corrugated_Metal_Panel_Opacity.exr");
}

TESTCASE(Pipe_client_ctl_startup_config_output_port_file_map_to_jopp_object)
{
	auto const result = Pipe::client_ctl::to_jopp_object(
		Pipe::client_ctl::output_port_file_map{
			{
				"port_1",
				{
					"Concrete_Scuffed_Industrial_BaseColor.exr",
					"Steel_Galvanized_Rusted_Metallic.exr"
				}
			},
			{
				"port_2",
				{
					"Steel_Galvanized_Rusted_Metallic.exr",
					"Brick_Victorian_Red_Normal.exr",
					"Asphalt_Wet_Cracked_Roughness.exr"
				}
			}
		}
	);

	auto const& port_1 = result.get_field_as<jopp::array>("port_1");
	REQUIRE_EQ(std::size(port_1), 2);
	EXPECT_EQ(port_1[0].get<std::string>(), "Concrete_Scuffed_Industrial_BaseColor.exr");
	EXPECT_EQ(port_1[1].get<std::string>(), "Steel_Galvanized_Rusted_Metallic.exr");

	auto const& port_2 = result.get_field_as<jopp::array>("port_2");
	REQUIRE_EQ(std::size(port_2), 3);
	EXPECT_EQ(port_2[0].get<std::string>(), "Steel_Galvanized_Rusted_Metallic.exr");
	EXPECT_EQ(port_2[1].get<std::string>(), "Brick_Victorian_Red_Normal.exr");
	EXPECT_EQ(port_2[2].get<std::string>(), "Asphalt_Wet_Cracked_Roughness.exr");
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_output_port_file_map)
{
	jopp::object obj;
	{
		jopp::array port;
		port.push_back("Concrete_Scuffed_Industrial_BaseColor.exr");
		port.push_back("Steel_Galvanized_Rusted_Metallic.exr");
		obj.insert("port_1", std::move(port));
	}

	{
		jopp::array port;
		port.push_back("Steel_Galvanized_Rusted_Metallic.exr");
		port.push_back("Brick_Victorian_Red_Normal.exr");
		port.push_back("Asphalt_Wet_Cracked_Roughness.exr");
		obj.insert("port_2", std::move(port));
	}

	auto const result = Pipe::client_ctl::make_output_port_file_map(obj);
	auto const& port_1 = result.at("port_1");
	REQUIRE_EQ(std::size(port_1), 2);
	EXPECT_EQ(port_1[0], "Concrete_Scuffed_Industrial_BaseColor.exr");
	EXPECT_EQ(port_1[1], "Steel_Galvanized_Rusted_Metallic.exr");

	auto const& port_2 = result.at("port_2");
	REQUIRE_EQ(std::size(port_2), 3);
	EXPECT_EQ(port_2[0], "Steel_Galvanized_Rusted_Metallic.exr");
	EXPECT_EQ(port_2[1], "Brick_Victorian_Red_Normal.exr");
	EXPECT_EQ(port_2[2], "Asphalt_Wet_Cracked_Roughness.exr");
}

TESTCASE(Pipe_client_ctl_startup_config_local_config_to_jopp_object)
{
	auto const result = to_jopp_object(
		Pipe::client_ctl::local_config{
			.inputs = Pipe::client_ctl::input_port_file_map{
				{"port_1", "Brick_Victorian_Red_Normal.exr"},
			},
			.outputs = Pipe::client_ctl::output_port_file_map{
				{"port_1", {"Concrete_Scuffed_Industrial_BaseColor.exr"}},
				{"port_2", {"Steel_Galvanized_Rusted_Metallic.exr", "Asphalt_Wet_Cracked_Roughness.exr"}}
			}
		}
	);

	{
		auto const& inputs = result.get_field_as<jopp::object>("inputs");
		auto const& port_1 = inputs.get_field_as<std::string>("port_1");
		EXPECT_EQ(port_1, "Brick_Victorian_Red_Normal.exr");
	}

	{
		auto const& outputs = result.get_field_as<jopp::object>("outputs");

		auto const& port_1 = outputs.get_field_as<jopp::array>("port_1");
		REQUIRE_EQ(std::size(port_1), 1);
		EXPECT_EQ(port_1[0].get<std::string>(), "Concrete_Scuffed_Industrial_BaseColor.exr");

		auto const& port_2 = outputs.get_field_as<jopp::array>("port_2");
		REQUIRE_EQ(std::size(port_2), 2);
		EXPECT_EQ(port_2[0].get<std::string>(), "Steel_Galvanized_Rusted_Metallic.exr");
		EXPECT_EQ(port_2[1].get<std::string>(), "Asphalt_Wet_Cracked_Roughness.exr");
	}
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_local_config)
{
	jopp::object obj;
	{
		jopp::object inputs;
		inputs.insert("port_1", "Brick_Victorian_Red_Normal.exr");
		obj.insert("inputs", std::move(inputs));
	}

	{
		jopp::object outputs;
		{
			jopp::array port;
			port.push_back("Concrete_Scuffed_Industrial_BaseColor.exr");
			outputs.insert("port_1", std::move(port));
		}

		{
			jopp::array port;
			port.push_back("Steel_Galvanized_Rusted_Metallic.exr");
			port.push_back("Asphalt_Wet_Cracked_Roughness.exr");
			outputs.insert("port_2", std::move(port));
		}
		obj.insert("outputs", std::move(outputs));
	}

	auto const result = Pipe::client_ctl::make_local_config(obj);

	{
		auto const& inputs = result.inputs;
		auto const& port_1 = inputs.at("port_1");
		EXPECT_EQ(port_1, "Brick_Victorian_Red_Normal.exr");
	}

	{
		auto const& outputs = result.outputs;

		auto const& port_1 = outputs.at("port_1");
		REQUIRE_EQ(std::size(port_1), 1);
		EXPECT_EQ(port_1[0], "Concrete_Scuffed_Industrial_BaseColor.exr");

		auto const& port_2 = outputs.at("port_2");
		REQUIRE_EQ(std::size(port_2), 2);
		EXPECT_EQ(port_2[0], "Steel_Galvanized_Rusted_Metallic.exr");
		EXPECT_EQ(port_2[1], "Asphalt_Wet_Cracked_Roughness.exr");
	}
}

TESTCASE(Pipe_client_ctl_startup_config_startup_config_local_config_to_jopp_object)
{
	auto const result = to_jopp_object(
		Pipe::client_ctl::startup_config{
			Pipe::client_ctl::local_config{
				.inputs = Pipe::client_ctl::input_port_file_map{
					{"port_1", "Brick_Victorian_Red_Normal.exr"},
				},
				.outputs = Pipe::client_ctl::output_port_file_map{
					{"port_1", {"Concrete_Scuffed_Industrial_BaseColor.exr"}},
					{"port_2", {"Steel_Galvanized_Rusted_Metallic.exr", "Asphalt_Wet_Cracked_Roughness.exr"}}
				}
			}
		}
	);

	EXPECT_EQ(result.get_field_as<std::string>("operational_mode"), "standalone");

	auto const& params = result.get_field_as<jopp::object>("parameters");

	{
		auto const& inputs = params.get_field_as<jopp::object>("inputs");
		auto const& port_1 = inputs.get_field_as<std::string>("port_1");
		EXPECT_EQ(port_1, "Brick_Victorian_Red_Normal.exr");
	}

	{
		auto const& outputs = params.get_field_as<jopp::object>("outputs");

		auto const& port_1 = outputs.get_field_as<jopp::array>("port_1");
		REQUIRE_EQ(std::size(port_1), 1);
		EXPECT_EQ(port_1[0].get<std::string>(), "Concrete_Scuffed_Industrial_BaseColor.exr");

		auto const& port_2 = outputs.get_field_as<jopp::array>("port_2");
		REQUIRE_EQ(std::size(port_2), 2);
		EXPECT_EQ(port_2[0].get<std::string>(), "Steel_Galvanized_Rusted_Metallic.exr");
		EXPECT_EQ(port_2[1].get<std::string>(), "Asphalt_Wet_Cracked_Roughness.exr");
	}
}

TESTCASE(Pipe_client_ctl_startup_config_startup_config_hostinfo_to_jopp_object)
{
	auto const result = to_jopp_object(
		Pipe::client_ctl::startup_config{
			Pipe::client_ctl::host_info{
				.address = Pipe::client_ctl::host_address{324}
			}
		}
	);

	EXPECT_EQ(result.get_field_as<std::string>("operational_mode"), "connected_to_host");

	auto const& params = result.get_field_as<jopp::object>("parameters");
	auto const& address = params.get_field_as<jopp::object>("address");
	auto const& address_type = address.get_field_as<jopp::string>("type");
	EXPECT_EQ(address_type, "socket_fd");
	EXPECT_EQ(address.get_field_as<double>("value"), 324.0);
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_startup_config_unsupported_operational_mode)
{
	jopp::object obj;
	obj.insert("operational_mode", "foobar");

	try
	{
		auto result = Pipe::client_ctl::make_startup_config(obj);
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The given operational mode is not supported"});
	}
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_startup_config_connected_to_host)
{
	jopp::object obj;
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	obj.insert("operational_mode", "connected_to_host");

	jopp::object host_info;
	jopp::object host_address;
	host_address.insert("type", "socket_fd");
	host_address.insert("value", static_cast<double>(sockets.socket_a().native_handle()));
	host_info.insert("address", std::move(host_address));
	obj.insert("parameters", std::move(host_info));

	auto const result = Pipe::client_ctl::make_startup_config(obj);
	EXPECT_EQ(
		std::get<Pipe::client_ctl::socket_fd_ref>(
			std::get<Pipe::client_ctl::host_info>(result).address
		).native_handle(),
		sockets.socket_a().native_handle()
	);
}

TESTCASE(Pipe_client_ctl_startup_config_jopp_object_to_startup_config_standalone)
{
	jopp::object obj;
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	obj.insert("operational_mode", "standalone");


	jopp::object local_config;
	{
		jopp::object inputs;
		inputs.insert("port_1", "Brick_Victorian_Red_Normal.exr");
		local_config.insert("inputs", std::move(inputs));
	}

	{
		jopp::object outputs;
		{
			jopp::array port;
			port.push_back("Concrete_Scuffed_Industrial_BaseColor.exr");
			outputs.insert("port_1", std::move(port));
		}

		{
			jopp::array port;
			port.push_back("Steel_Galvanized_Rusted_Metallic.exr");
			port.push_back("Asphalt_Wet_Cracked_Roughness.exr");
			outputs.insert("port_2", std::move(port));
		}
		local_config.insert("outputs", std::move(outputs));
	}
	obj.insert("parameters", std::move(local_config));

	auto const result = Pipe::client_ctl::make_startup_config(obj);
	auto const params = std::get<Pipe::client_ctl::local_config>(result);

	{
		auto const& inputs = params.inputs;
		auto const& port_1 = inputs.at("port_1");
		EXPECT_EQ(port_1, "Brick_Victorian_Red_Normal.exr");
	}

	{
		auto const& outputs = params.outputs;

		auto const& port_1 = outputs.at("port_1");
		REQUIRE_EQ(std::size(port_1), 1);
		EXPECT_EQ(port_1[0], "Concrete_Scuffed_Industrial_BaseColor.exr");

		auto const& port_2 = outputs.at("port_2");
		REQUIRE_EQ(std::size(port_2), 2);
		EXPECT_EQ(port_2[0], "Steel_Galvanized_Rusted_Metallic.exr");
		EXPECT_EQ(port_2[1], "Asphalt_Wet_Cracked_Roughness.exr");
	}
}