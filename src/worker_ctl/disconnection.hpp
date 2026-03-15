#ifndef PIPE_WORKER_CTL_DISCONNECTION_HPP
#define PIPE_WORKER_CTL_DISCONNECTION_HPP

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	template<class Tag>
	struct disconnection
	{
		std::string portname;
	};

	template<class Tag>
	inline jopp::object to_jopp_object(disconnection<Tag>&& obj)
	{
		jopp::object ret;
		ret.insert("portname", std::move(obj.portname));
		return ret;
	}

	template<class Tag>
	inline disconnection<Tag> make_disconnection(jopp::object&& obj)
	{
		return disconnection<Tag>{
			.portname = std::move(obj.get_field_as<jopp::string>("portname"))
		};
	}

	struct input_port_tag{};

	using input_disconnection = disconnection<input_port_tag>;

	inline input_disconnection make_input_disconnection(jopp::object&& obj)
	{ return make_disconnection<input_port_tag>(std::move(obj)); }

	struct output_port_tag{};

	using output_disconnection = disconnection<output_port_tag>;

	inline output_disconnection make_output_disconnection(jopp::object&& obj)
	{ return make_disconnection<output_port_tag>(std::move(obj)); }
}
#endif