//@	{"target":{"name":"testsuite.o"}}

#include "src/os_services/ipc/socket_pair.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/json_log/item_converter.hpp"
#include "src/client_ctl/startup_config.hpp"

#include <jopp/parser.hpp>
#include <jopp/serializer.hpp>
#include <testfwk/testfwk.hpp>

namespace
{
	std::filesystem::path testclient_exe()
	{
		return std::filesystem::path{MAIKE_BUILDINFO_TARGETDIR}/"src/client/test/client";
	}

	Pipe::log::item fetch_log_item(Pipe::os_services::io::input_file_descriptor_ref output)
	{
		std::array<char, 4096> readbuf{};
		jopp::container parsed_log_item;
		jopp::parser log_parser{parsed_log_item};
		while(true)
		{
			auto const read_result = read(output, std::as_writable_bytes(std::span{readbuf}));
			if(read_result.bytes_transferred() != 0)
			{
				auto const parse_result = log_parser.parse(
					std::span{std::data(readbuf), read_result.bytes_transferred()}
				);
				if(parse_result.ec == jopp::parser_error_code::completed)
				{ return Pipe::json_log::make_log_item(parsed_log_item.get<jopp::object>()).value(); }
				if(parse_result.ec != jopp::parser_error_code::more_data_needed)
				{
					throw std::runtime_error{
						std::format("Bad json data returned from child: {}", to_string(parse_result.ec))
					};
				}
			}

			if(read_result.bytes_transferred() == 0)
			{ return Pipe::log::item{}; }
		}
	}
}

TESTCASE(Pipe_client_main_to_few_args)
{
	auto const exe_file = testclient_exe();
	Pipe::os_services::ipc::pipe proc_output;

	auto const res = Pipe::os_services::proc_mgmt::spawn(
		exe_file.c_str(),
		std::span<char const*>{},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = proc_output.take_write_end()
		}
	);

	auto const log_item = fetch_log_item(proc_output.read_end());
	EXPECT_EQ(log_item.severity, Pipe::log::item::severity::error);
	EXPECT_EQ(log_item.message, "Wrong number of command line arguments. Got 1 expected 2");

	auto const proc_res = Pipe::os_services::proc_mgmt::wait(res.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_res).return_value, 255);
}

TESTCASE(Pipe_client_main_to_many_args)
{
	auto const exe_file = testclient_exe();
	Pipe::os_services::ipc::pipe proc_output;

	std::array<char const*, 2> args{"foo", "bar"};
	auto const res = Pipe::os_services::proc_mgmt::spawn(
		exe_file.c_str(),
		std::span<char const*>{args},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = proc_output.take_write_end()
		}
	);

	auto const log_item = fetch_log_item(proc_output.read_end());
	EXPECT_EQ(log_item.severity, Pipe::log::item::severity::error);
	EXPECT_EQ(log_item.message, "Wrong number of command line arguments. Got 3 expected 2");

	auto const proc_res = Pipe::os_services::proc_mgmt::wait(res.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_res).return_value, 255);
}

TESTCASE(Pipe_client_main_sucessful_start)
{
	auto const exe_file = testclient_exe();
	Pipe::os_services::ipc::pipe proc_output;
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;

	auto const startup_config = to_string(
		Pipe::client_ctl::to_jopp_object(
			Pipe::client_ctl::startup_config{
				Pipe::client_ctl::host_info{
					.address = sockets.socket_b()
				}
			}
		)
	);

	std::array fds_to_keep{Pipe::os_services::fd::file_descriptor{sockets.take_socket_b().release()}};
	std::array args_cstr{startup_config.c_str()};

	auto const res = Pipe::os_services::proc_mgmt::spawn(
		exe_file.c_str(),
		std::span<char const*>{args_cstr},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = proc_output.take_write_end()
		},
		std::span{fds_to_keep}
	);

	auto const log_item = fetch_log_item(proc_output.read_end());
	EXPECT_EQ(log_item.severity, Pipe::log::item::severity::info);
	EXPECT_EQ(log_item.message, "Process exited normally");

	auto const proc_res = Pipe::os_services::proc_mgmt::wait(res.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_res).return_value, 0);
}