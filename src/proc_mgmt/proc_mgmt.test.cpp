//@	{"target":{"name": "proc_mgmt.test"}}

#include "./proc_mgmt.hpp"

#include <testfwk/testfwk.hpp>
#include <thread>

TESTCASE(prog_proc_mgmt_spawn_program_not_found)
{
	try
	{
		auto const proc = prog::proc_mgmt::spawn(
			"this_program_does_not_exist",
			std::span<char const*>{},
			std::span<char const*>{},
			prog::proc_mgmt::io_redirection{}
		);
	}
	catch(std::exception const& e)
	{
		EXPECT_EQ(
			e.what(),
			std::string_view{"Failed to launch application this_program_does_not_exist: No such file or directory"}
		);
	}
}

TESTCASE(prog_proc_mgmt_spawn_run_succesful_program)
{
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/test",
		std::span<char const*>{},
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{}
	);

	auto res = wait(proc.get());
	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(res).return_value, 1);
}
