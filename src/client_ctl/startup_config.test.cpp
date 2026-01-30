//@	{"target": {"name": "startup_config.test"}}

#include "./startup_config.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_client_ctl_startup_config_socket_fd_ref_to_jopp_value)
{
	auto const result = Pipe::client_ctl::to_jopp_value(Pipe::client_ctl::socket_fd_ref{346});
	EXPECT_EQ(result, 346.0);
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