//@	{"target":{"name": "proc_mgmt.test"}}

#include "./proc_mgmt.hpp"
#include "src/ipc/pipe.hpp"
#include "testfwk/validation.hpp"

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

TESTCASE(prog_proc_mgmt_spawn_run_expect_exit_status_1)
{
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/false",
		std::span<char const*>{},
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{}
	);

	auto res = wait(proc.get());
	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(res).return_value, 1);
}

TESTCASE(prog_proc_mgmt_spawn_run_expect_exit_status_0)
{
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/true",
		std::span<char const*>{},
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{}
	);

	auto res = wait(proc.get());
	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(res).return_value, 0);
}

TESTCASE(prog_proc_mgmt_spawn_run_with_args)
{
	prog::ipc::pipe stdout_pipe;
	std::array<char const*, 4> args{"This", "is", "a", "test"};
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/echo",
		args,
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = stdout_pipe.write_end(),
			.syserr = {}
		}
	);

	std::array<char, 32> buffer{};
	auto read_result = read(stdout_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 15);
	EXPECT_EQ((std::string_view{std::data(buffer), 15}), "This is a test\n");

	auto proc_result = wait(proc.get());
	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(prog_proc_mgmt_spawn_run_with_env)
{
	prog::ipc::pipe stdout_pipe;
	std::array<char const*, 4> env{"FOO=bar", "X=kaka"};
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/env",
		std::span<char const*>{},
		env,
		prog::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = stdout_pipe.write_end(),
			.syserr = {}
		}
	);

	std::array<char, 32> buffer{};
	auto const read_result = read(stdout_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 15);
	EXPECT_EQ((std::string_view{std::data(buffer), 15}), "FOO=bar\nX=kaka\n");

	auto const proc_result = wait(proc.get());	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(prog_proc_mgmt_spawn_run_pass_through)
{
	prog::ipc::pipe stdout_pipe;
	prog::ipc::pipe stdin_pipe;
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/cat",
		std::span<char const*>{},
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{
			.sysin = stdin_pipe.read_end(),
			.sysout = stdout_pipe.write_end(),
			.syserr = {}
		}
	);

	auto const write_result = write(
		stdin_pipe.write_end(),
		std::as_bytes(std::span{std::string_view{"Hello, World"}})
	);
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	std::array<char, 32> buffer{};
	auto const read_result = read(stdout_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 12);
	stdin_pipe.close_write_end();

	auto const proc_result = wait(proc.get());	EXPECT_EQ(std::get<prog::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(prog_proc_mgmt_spawn_run_kill)
{
	auto const proc = prog::proc_mgmt::spawn(
		"/usr/bin/cat",
		std::span<char const*>{},
		std::span<char const*>{},
		prog::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = {}
		}
	);

	kill(proc.get(), SIGTERM);
	auto const proc_result = wait(proc.get());	EXPECT_EQ(std::get<prog::proc_mgmt::process_killed>(proc_result).signo, SIGTERM);
}