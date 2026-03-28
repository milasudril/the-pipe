#ifndef PIPE_WORKER_CTL_STREAM_INFO_HPP
#define PIPE_WORKER_CTL_STREAM_INFO_HPP

#include "./any.hpp"

namespace Pipe::worker_ctl
{
	struct data_format_info_tag{};

	using data_format_info = any<data_format_info_tag>;

	inline data_format_info make_data_format_info(jopp::object&& obj)
	{ return make_any<data_format_info_tag>(std::move(obj)); }

	struct data_transport_info_tag{};

	using data_transport_info = any<data_transport_info_tag>;

	inline data_transport_info make_data_transport_info(jopp::object&& obj)
	{ return make_any<data_transport_info_tag>(std::move(obj)); }

	struct data_stream_info
	{
		data_format_info format;
		data_transport_info transport_params;
	};

	inline jopp::object to_jopp_object(data_stream_info&& obj)
	{
		jopp::object ret;
		ret.insert("format", to_jopp_object(std::move(obj.format)));
		ret.insert("transport_params", to_jopp_object(std::move(obj.transport_params)));
		return ret;
	}

	inline data_stream_info make_data_stream_info(jopp::object&& obj)
	{
		return data_stream_info{
			.format = make_data_format_info(std::move(obj.get_field_as<jopp::object>("format"))),
			.transport_params = make_data_transport_info(
				std::move(obj.get_field_as<jopp::object>("transport_params"))
			)
		};
	}

	struct port_capability
	{
		data_format_info format;
		type_descriptor transport_method;
	};

	inline jopp::object to_jopp_object(port_capability&& obj)
	{
		jopp::object ret;
		ret.insert("format", to_jopp_object(std::move(obj.format)));
		ret.insert("transport_method", to_jopp_object(std::move(obj.transport_method)));
		return ret;
	}

	inline port_capability make_port_capability(jopp::object&& obj)
	{
		return port_capability{
			.format = make_data_format_info(std::move(obj.get_field_as<jopp::object>("format"))),
			.transport_method = make_type_descriptor(std::move(obj.get_field_as<jopp::object>("transport_method")))
		};
	}
}

#endif