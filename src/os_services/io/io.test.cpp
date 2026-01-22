//@	{"target":{"name":"io.test"}}

#include "./io.hpp"
#include "src/os_services/fd/file_descriptor.hpp"

#include <cerrno>
#include <testfwk/testfwk.hpp>
#include <sys/mman.h>

namespace
{
	struct memfd_tag
	{
	};
}

template<>
struct Pipe::os_services::fd::enabled_fd_conversions<memfd_tag>
{
	static consteval void supports(Pipe::os_services::io::input_file_descriptor_tag){}
	static consteval void supports(Pipe::os_services::io::output_file_descriptor_tag){}
};

TESTCASE(Pipe_io_read_zero_bytes_available)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", 0)};
	REQUIRE_NE(fd, nullptr);

	std::array<std::byte, 4096> buffer{};
	auto const res = Pipe::os_services::io::read(fd.get(), buffer);
	EXPECT_EQ(res.operation_would_have_blocked(), false);
	EXPECT_EQ(res.bytes_transferred(), 0);
}

TESTCASE(Pipe_io_write_and_read_succesful)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", 0)};
	REQUIRE_NE(fd, nullptr);
	REQUIRE_NE(fd.get().native_handle(), STDOUT_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDIN_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDERR_FILENO);

	std::string_view value_to_write{"Hello, World"};

	auto const write_result = Pipe::os_services::io::write(fd.get(), std::as_bytes(std::span{value_to_write}));
	EXPECT_EQ(write_result.operation_would_have_blocked(), false);
	EXPECT_EQ(write_result.bytes_transferred(), std::size(value_to_write));

	auto const seek_res = ::lseek(fd.get().native_handle(), 0, SEEK_SET);
	if(seek_res == -1)
	{ perror("lseek failed"); }
	REQUIRE_NE(seek_res, -1);

	std::array<char, 4096> buffer{};
	auto const read_result = Pipe::os_services::io::read(fd.get(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.operation_would_have_blocked(), false);
	EXPECT_EQ(read_result.bytes_transferred(), std::size(value_to_write));
}

TESTCASE(Pipe_io_read_bad_fd)
{
	try
	{
		std::ignore = read(Pipe::os_services::io::input_file_descriptor_ref{-1}, std::span<std::byte>{});
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"I/O operation failed: Bad file descriptor"});
	}
}

TESTCASE(Pipe_io_io_result_from_eagain)
{
	{
		Pipe::os_services::io::io_result res{-1, EAGAIN};
		EXPECT_EQ(res.operation_would_have_blocked(), true);
		EXPECT_EQ(res.bytes_transferred(), 0);
	}

	{
		Pipe::os_services::io::io_result res{-1, EWOULDBLOCK};
		EXPECT_EQ(res.operation_would_have_blocked(), true);
		EXPECT_EQ(res.bytes_transferred(), 0);
	}
}