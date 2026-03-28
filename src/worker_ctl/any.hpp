#ifndef PIPE_WORKER_CTL_ANY_HPP
#define PIPE_WORKER_CTL_ANY_HPP

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	struct type_descriptor
	{
		std::vector<std::string> ns_path;
		std::string name;
	};

	inline jopp::object to_jopp_object(type_descriptor&& obj)
	{
		jopp::object ret;
		jopp::array ns_path;
		for(auto& item: obj.ns_path)
		{ ns_path.push_back(std::move(item)); }

		ret.insert("ns_path", std::move(ns_path));
		ret.insert("name", std::move(obj.name));
		return ret;
	}

	inline type_descriptor make_type_descriptor(jopp::object&& obj)
	{
		std::vector<std::string> ns_path;
		for(auto& item : obj.get_field_as<jopp::array>("ns_path"))
		{ ns_path.push_back(std::move(item.get<jopp::string>())); }

		return type_descriptor{
			.ns_path = std::move(ns_path),
			.name = std::move(obj.get_field_as<jopp::string>("name"))
		};
	}

	template<class Tag>
	struct any
	{
		type_descriptor type;
		jopp::value value;
	};

	template<class Tag>
	inline jopp::object to_jopp_object(any<Tag>&& obj)
	{
		jopp::object ret;
		ret.insert("type", to_jopp_object(std::move(obj.type)));
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	template<class Tag>
	inline any<Tag> make_any(jopp::object&& obj)
	{
		auto const i = obj.find("value");
		return any<Tag>{
			.type = make_type_descriptor(std::move(obj.get_field_as<jopp::object>("type"))),
			.value = (i != std::end(obj))? std::move(i->second) : jopp::value{}
		};
	}
}

#endif