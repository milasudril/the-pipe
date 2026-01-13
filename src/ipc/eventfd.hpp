#ifndef PROG_IPC_EVENTFD_HPP
#define PROG_IPC_EVENTFD_HPP

#include "src/io/io.hpp"
#include "src/utils/system_error.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <sys/eventfd.h>

namespace prog::ipc
{
	struct eventfd_tag
	{};

	using eventfd_ref = utils::tagged_file_descriptor_ref<eventfd_tag>;
	using eventfd = utils::tagged_file_descriptor<eventfd_tag>;

	auto make_eventfd()
	{
		eventfd ret{::eventfd(0, 0)};
		if(ret == nullptr)
		{ throw utils::system_error{"Failed to create eventfd", errno}; }
		return ret;
	}
}

#endif