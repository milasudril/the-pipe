#ifndef PIPE_WORKER_CTL_STREAM_INFO_HPP
#define PIPE_WORKER_CTL_STREAM_INFO_HPP

#include "./data_format_info.hpp"

namespace Pipe::worker_ctl
{
	struct data_framing_info_tag{};

	using data_framing_info = any<data_framing_info_tag>;

	inline data_framing_info make_data_framing_info(jopp::object&& obj)
	{ return make_any<data_framing_info_tag>(std::move(obj)); }

	struct data_stream_info
	{
		data_format_info format;
		data_framing_info framing;
	};

	inline jopp::object to_jopp_object(data_stream_info&& obj)
	{
		jopp::object ret;
		ret.insert("format", to_jopp_object(std::move(obj.format)));
		ret.insert("framing", to_jopp_object(std::move(obj.framing)));
		return ret;
	}

	inline data_stream_info make_data_stream_info(jopp::object&& obj)
	{
		return data_stream_info{
			.format = make_data_format_info(std::move(obj.get_field_as<jopp::object>("format"))),
			.framing = make_data_framing_info(std::move(obj.get_field_as<jopp::object>("framing")))
		};
	}
}

#endif