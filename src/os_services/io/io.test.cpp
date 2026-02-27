//@	{"target":{"name":"io.test"}}

#include "./io.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "testfwk/validation.hpp"

#include <cerrno>
#include <testfwk/testfwk.hpp>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{
	struct memfd_tag
	{
	};

	size_t max_ío_size = 4096;
}

extern "C"
{
	ssize_t read(int fd, void* buffer, size_t count)
	{
		return syscall(SYS_read, fd, buffer, std::min(max_ío_size, count));
	}

	ssize_t write(int fd, void const* buffer, size_t count)
	{
		auto ret =  syscall(SYS_write, fd, buffer, std::min(max_ío_size, count));
		// HACK: Makes it possible to fake EAGAIN by setting a seal
		if(ret == -1 && errno == EPERM)
		{ errno = EAGAIN; }
		return ret;
	}
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

TESTCASE(Pipe_io_read_full_end_before_buffer_end)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", 0)};
	REQUIRE_NE(fd, nullptr);
	REQUIRE_NE(fd.get().native_handle(), STDOUT_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDIN_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDERR_FILENO);

	static constexpr std::string_view value_to_write{"Hello, World"};

	auto const write_result = Pipe::os_services::io::write(fd.get(), std::as_bytes(std::span{value_to_write}));
	EXPECT_EQ(write_result.operation_would_have_blocked(), false);
	EXPECT_EQ(write_result.bytes_transferred(), std::size(value_to_write));

	auto const seek_res = ::lseek(fd.get().native_handle(), 0, SEEK_SET);
	if(seek_res == -1)
	{ perror("lseek failed"); }
	REQUIRE_NE(seek_res, -1);

	max_ío_size = 7;
	std::array<char, 2*std::size(value_to_write)> buffer{};
	auto const read_result = Pipe::os_services::io::read_full(fd.get(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.operation_would_have_blocked(), false);
	EXPECT_EQ(read_result.bytes_transferred(), std::size(value_to_write));

	max_ío_size = 4096;
}

TESTCASE(Pipe_io_read_full_complete_block)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", 0)};
	REQUIRE_NE(fd, nullptr);
	REQUIRE_NE(fd.get().native_handle(), STDOUT_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDIN_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDERR_FILENO);

	static constexpr std::string_view value_to_write{"Hello, World"};

	auto const write_result = Pipe::os_services::io::write(fd.get(), std::as_bytes(std::span{value_to_write}));
	EXPECT_EQ(write_result.operation_would_have_blocked(), false);
	EXPECT_EQ(write_result.bytes_transferred(), std::size(value_to_write));

	auto const seek_res = ::lseek(fd.get().native_handle(), 0, SEEK_SET);
	if(seek_res == -1)
	{ perror("lseek failed"); }
	REQUIRE_NE(seek_res, -1);

	max_ío_size = 7;
	std::array<char, std::size(value_to_write)> buffer{};
	auto const read_result = Pipe::os_services::io::read_full(fd.get(), std::as_writable_bytes(std::span{buffer}));
	EXPECT_EQ(read_result.operation_would_have_blocked(), false);
	EXPECT_EQ(read_result.bytes_transferred(), std::size(value_to_write));

	max_ío_size = 4096;
}

TESTCASE(Pipe_io_write_full_end_before_buffer_end)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", MFD_ALLOW_SEALING)};
	static constexpr std::string_view value_to_write{"Hello, World"};
	auto const res = ftruncate(fd.get().native_handle(), std::size(value_to_write) - 1);
	REQUIRE_NE(res, -1);
	fcntl(fd.get().native_handle(), F_ADD_SEALS, F_SEAL_GROW);
	REQUIRE_NE(fd, nullptr);
	REQUIRE_NE(fd.get().native_handle(), STDOUT_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDIN_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDERR_FILENO);

	max_ío_size = 7;
	auto const write_result = Pipe::os_services::io::write_full(fd.get(), std::as_bytes(std::span{value_to_write}));
	EXPECT_EQ(write_result.operation_would_have_blocked(), true);
	EXPECT_EQ(write_result.bytes_transferred(), max_ío_size);

	max_ío_size = 4096;
}

TESTCASE(Pipe_io_write_full_complete_block)
{
	Pipe::os_services::fd::tagged_file_descriptor<memfd_tag> fd{memfd_create("foo", MFD_ALLOW_SEALING)};
	static constexpr std::string_view value_to_write{"Hello, World"};
	REQUIRE_NE(fd, nullptr);
	REQUIRE_NE(fd.get().native_handle(), STDOUT_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDIN_FILENO);
	REQUIRE_NE(fd.get().native_handle(), STDERR_FILENO);

	max_ío_size = 7;
	auto const write_result = Pipe::os_services::io::write_full(fd.get(), std::as_bytes(std::span{value_to_write}));
	EXPECT_EQ(write_result.operation_would_have_blocked(), false);
	EXPECT_EQ(write_result.bytes_transferred(), std::size(value_to_write));

	max_ío_size = 4096;
}