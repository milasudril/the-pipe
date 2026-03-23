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
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	template<class Tag, class TypeNameType>
	inline jopp::object to_jopp_object(any<Tag, std::vector<TypeNameType>>&& obj)
	{
		jopp::object ret;
		jopp::array type;
		for(auto& obj : obj.type)
		{ type.push_back(std::move(obj)); }

		ret.insert("type", std::move(type));
		ret.insert("value", std::move(obj.value));
		return ret;
	}

	template<class Tag, class TypeNameType>
	struct make_any_impl
	{
		static any<Tag, TypeNameType> make_any(jopp::object&& obj)
		{
			any<Tag, TypeNameType> ret;
			ret.type = std::move(obj.get_field_as<TypeNameType>("type"));

			auto value = obj.find("value");
			if(value != std::end(obj))
			{ ret.value = std::move(value->second); }

			return ret;
		}
	};

	template<class Tag, class TypeNameType>
	struct make_any_impl<Tag, std::vector<TypeNameType>>
	{
		static any<Tag, std::vector<TypeNameType>> make_any(jopp::object&& obj)
		{
			any<Tag, std::vector<TypeNameType>> ret;
			auto& type = obj.get_field_as<jopp::array>("type");
			for(auto& item : type)
			{ ret.type.push_back(std::move(item.get<TypeNameType>())); }

			auto value = obj.find("value");
			if(value != std::end(obj))
			{ ret.value = std::move(value->second); }

			return ret;
		}
	};

	template<class Tag, class TypeNameType = std::string>
	inline any<Tag, TypeNameType> make_any(jopp::object&& obj)
	{
		return make_any_impl<Tag, TypeNameType>::make_any(std::move(obj));
	}
}

#endif