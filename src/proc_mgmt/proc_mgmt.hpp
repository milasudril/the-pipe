#ifndef PROG_PROC_MGMT_HPP
#define PROG_PROC_MGMT_HPP

#include "src/io/io.hpp"
#include "src/utils/file_descriptor.hpp"
#include <filesystem>

namespace prog::proc_mgmt
{
	struct pidfd_tag
	{};

	using pidfd_ref = utils::tagged_file_descriptor_ref<pidfd_tag>;
	using pidfd = utils::tagged_file_descriptor<pidfd_tag>;

	struct io_redirection
	{
		io::input_file_descriptor_ref sysin;
		io::output_file_descriptor_ref sysout;
		io::output_file_descriptor_ref syserr;
	};

	pidfd spawn(
		char const* path,
		std::span<char const*> argv,
		std::span<char const*> env,
		io_redirection const& io_redir
	);
};

#endif
