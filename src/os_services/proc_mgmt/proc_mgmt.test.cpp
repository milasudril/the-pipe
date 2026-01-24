//@	{"target": {"name": "proc_mgmt.test"}}

#include "./proc_mgmt.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket_pair.hpp"

#include <cstdlib>
#include <ranges>
#include <testfwk/testfwk.hpp>
#include <thread>

TESTCASE(Pipe_proc_mgmt_spawn_program_not_found)
{
	try
	{
		auto const proc = Pipe::os_services::proc_mgmt::spawn(
			"this_program_does_not_exist",
			std::span<char const*>{},
			std::span<char const*>{},
			Pipe::os_services::proc_mgmt::io_redirection{}
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

TESTCASE(Pipe_proc_mgmt_spawn_run_expect_exit_status_1)
{
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/false",
		std::span<char const*>{},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{}
	);

	auto res = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(res).return_value, 1);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_expect_exit_status_0)
{
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/true",
		std::span<char const*>{},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{}
	);

	auto res = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(res).return_value, 0);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_with_args)
{
	Pipe::os_services::ipc::pipe stdout_pipe;
	std::array<char const*, 4> args{"This", "is", "a", "test"};
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/echo",
		args,
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = stdout_pipe.take_write_end(),
			.syserr = {}
		}
	);

	std::array<char, 32> buffer{};
	auto read_result = read(stdout_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 15);
	EXPECT_EQ((std::string_view{std::data(buffer), 15}), "This is a test\n");

	auto proc_result = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_with_env)
{
	Pipe::os_services::ipc::pipe stdout_pipe;
	std::array<char const*, 4> env{"FOO=bar", "X=kaka"};
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/env",
		std::span<char const*>{},
		env,
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = stdout_pipe.take_write_end(),
			.syserr = {}
		}
	);

	std::array<char, 32> buffer{};
	auto const read_result = read(stdout_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 15);
	EXPECT_EQ((std::string_view{std::data(buffer), 15}), "FOO=bar\nX=kaka\n");

	auto const proc_result = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_pass_through)
{
	Pipe::os_services::ipc::pipe stdout_pipe;
	Pipe::os_services::ipc::pipe stdin_pipe;
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/cat",
		std::span<char const*>{},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = stdin_pipe.take_read_end(),
			.sysout = stdout_pipe.take_write_end(),
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

	auto const proc_result = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_redirect_stderr)
{
	Pipe::os_services::ipc::pipe stderr_pipe;
	std::array<char const*, 4> args{"-c", ">&2 echo Hello, World"};
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/bash",
		args,
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = stderr_pipe.take_write_end()
		}
	);

	std::array<char, 32> buffer{};
	auto read_result = read(stderr_pipe.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.bytes_transferred(), 13);
	EXPECT_EQ((std::string_view{std::data(buffer), 13}), "Hello, World\n");

	auto proc_result = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_result).return_value, 0);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_kill)
{
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/cat",
		std::span<char const*>{},
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = {},
			.syserr = {}
		}
	);

	kill(proc.second.get(), SIGTERM);
	auto const proc_result = wait(proc.second.get());	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_killed>(proc_result).signo, SIGTERM);
}

TESTCASE(Pipe_proc_mgmt_spawn_run_with_extra_fd)
{
	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	std::array<char const*, 2> args{	"/proc/self/fd"};
	auto const fd_to_look_for = sockets.socket_b().native_handle();
	std::array fds_to_forward{Pipe::os_services::fd::file_descriptor{sockets.take_socket_b().release()}};
	Pipe::os_services::ipc::pipe sysout;
	auto const proc = Pipe::os_services::proc_mgmt::spawn(
		"/usr/bin/ls",
		args,
		std::span<char const*>{},
		Pipe::os_services::proc_mgmt::io_redirection{
			.sysin = {},
			.sysout = sysout.take_write_end(),
			.syserr = {}
		},
		fds_to_forward
	);

	std::array<char, 4096> buffer{};
	auto read_result = read(sysout.read_end(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.operation_would_have_blocked(), false);
	std::vector<int> open_fds;
	for(auto item : std::ranges::split_view{buffer, '\n'})
	{
		int output_value{-1};
		auto const res = std::from_chars(std::begin(item), std::end(item), output_value);
		if(res.ptr != std::end(item))
		{ break; }
		EXPECT_EQ(res.ptr, std::end(item));
		EXPECT_EQ(res.ec, std::errc{});
		open_fds.push_back(output_value);
	}

	auto proc_result = wait(proc.second.get());
	EXPECT_EQ(std::get<Pipe::os_services::proc_mgmt::process_exited>(proc_result).return_value, 0);

	static constexpr size_t num_std_fds = 3;
	static constexpr size_t self_fds = 1;
	static constexpr size_t num_extra_fds = 1;
	EXPECT_EQ(std::size(open_fds), num_std_fds + self_fds + num_extra_fds);
	EXPECT_NE(std::ranges::find(open_fds, STDIN_FILENO), std::end(open_fds));
	EXPECT_NE(std::ranges::find(open_fds, STDOUT_FILENO), std::end(open_fds));
	EXPECT_NE(std::ranges::find(open_fds, STDERR_FILENO), std::end(open_fds));
	EXPECT_NE(std::ranges::find(open_fds, fd_to_look_for), std::end(open_fds));
}
