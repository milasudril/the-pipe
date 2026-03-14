#ifndef PIPE_OS_SERVICES_FS_FS_HPP
#define PIPE_OS_SERVICES_FS_FS_HPP

#include "./file_access_permission.hpp"
#include "./file_open_precondition.hpp"

#include "src/os_services/error_handling/system_error.hpp"
#include "src/os_services/io/io.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <unistd.h>

namespace Pipe::os_services::fs
{
	inline constexpr int make_output_fd_oflags(file_open_precondition precond)
	{
		switch(precond)
		{
			case file_open_precondition::must_exist:
				return 0;
			case file_open_precondition::none:
				return O_CREAT;
			case file_open_precondition::must_not_exist:
				return O_CREAT | O_EXCL;
			default:
				throw std::range_error{""};
		}
	}

	inline io::output_file_descriptor make_output_fd(
		char const* path,
		file_open_precondition precond,
		file_access_permission perms
	)
	{
		auto fd = ::open(
			path,
			make_output_fd_oflags(precond) | O_WRONLY | O_TRUNC,
			static_cast<mode_t>(perms)
		);
		if(fd == -1)
		{ throw error_handling::system_error{std::format("Failed to open file {}", path), errno}; }

		return io::output_file_descriptor{fd};
	}

	inline io::input_file_descriptor make_input_fd(char const* path)
	{
		auto fd = ::open(path, O_RDONLY);
		if(fd == -1)
		{ throw error_handling::system_error{std::format("Failed to open file {}", path), errno}; }

		return io::input_file_descriptor{fd};
	}

	inline void make_fifo(char const* path, file_access_permission perms)
	{
		auto fd = mkfifo(path, static_cast<mode_t>(perms));
		if(fd == -1)
		{ throw error_handling::system_error{std::format("Failed to create fifo {}", path), errno}; }
		close(fd);
	}

	inline void remove(char const* path)
	{ std::ignore = ::unlink(path); }
}

#endif