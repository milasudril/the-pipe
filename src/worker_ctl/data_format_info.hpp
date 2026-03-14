#ifndef PIPE_WORKER_CTL_DATA_FORMAT_INFO_HPP
#define PIPE_WORKER_CTL_DATA_FORMAT_INFO_HPP

#include "./any.hpp"

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	struct data_format_info_tag{};

	using data_format_info = any<data_format_info_tag>;

	/**
	 * \brief Converts a jopp::object to a data_format_info
	 */
	inline data_format_info make_data_format_info(jopp::object&& obj)
	{ return make_any<data_format_info_tag>(std::move(obj)); }
}
#endif