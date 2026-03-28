#ifndef PIPE_WORKER_CTL_HPP
#define PIPE_WORKER_CTL_HPP

#include <jopp/types.hpp>

#include <string>
#include <vector>

/**
 * \brief Definitions for the protocol used to control workers
 */
namespace Pipe::worker_ctl
{
	struct worker_ctl_info
	{
		int protocol_version;
		std::vector<std::string> supported_methods;
	};

	inline jopp::object to_jopp_object(worker_ctl_info&& info)
	{
		jopp::object ret;
		ret.insert("protocol_version", static_cast<double>(info.protocol_version));
		jopp::array supported_methods;
		for(auto& item: info.supported_methods)
		{ supported_methods.push_back(std::move(item)); }

		ret.insert("supported_methods", std::move(supported_methods));
		return ret;
	}
}

#endif