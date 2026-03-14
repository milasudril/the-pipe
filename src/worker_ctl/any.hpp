#ifndef PIPE_WORKER_CTL_ANY_HPP
#define PIPE_WORKER_CTL_ANY_HPP

#include <jopp/types.hpp>

namespace Pipe::worker_ctl
{
	template<class Tag, class TypeNameType = std::string>
	struct any
	{
		TypeNameType type;
		jopp::value value;
	};

	template<class Tag, class TypeNameType>
	inline jopp::object to_jopp_object(any<Tag, TypeNameType>&& obj)
	{
		jopp::object ret;
		ret.insert("type", std::move(obj.type));
		// TODO: May need to convert value to string
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	template<class Tag, class TypeNameType = std::string>
	inline any<Tag, TypeNameType> make_any(jopp::object&& obj)
	{
		auto value = obj.find("value");
		if(value == std::end(obj))
		{ throw std::runtime_error{"Missing mandatory field `value`"}; }
		return any<Tag, TypeNameType>{
			// TODO: May need to convert from TypeNameType to string
			.type = std::move(obj.get_field_as<TypeNameType>("type")),
			.value = std::move(value->second)
		};
	}
}

#endif