#ifndef PIPE_UTILS_BOUND_MEMBER_FUNCTION_HPP
#define PIPE_UTILS_BOUND_MEMBER_FUNCTION_HPP

#include <bit>

namespace Pipe::utils
{
	template<auto Value>
	struct make_type
	{
		static constexpr auto value = Value;
	};

	template<class ReturnType, class ... ArgTypes>
	class bound_member_function
	{
	public:
		bound_member_function() = default;

		template<class ObjectType, ReturnType (ObjectType::*Callback)(ArgTypes...)>
		explicit bound_member_function(ObjectType& object, make_type<Callback>):
			m_object{&object},
			m_callback{[](void* object, ArgTypes... args){
				return (static_cast<ObjectType*>(object)->*make_type<Callback>::value)(args...);
			}}
		{}

		template<class ObjectType, ReturnType (*Callback)(ObjectType&, ArgTypes...)>
		explicit bound_member_function(ObjectType& object, make_type<Callback>):
			m_object{&object},
			m_callback{
				[](void* object, ArgTypes... args){
					return make_type<Callback>::value(*static_cast<ObjectType*>(object), args...);
				}
			}
		{}

		ReturnType operator()(ArgTypes... args) const
		{ return m_callback(m_object, args...); }

		bool operator==(bound_member_function const&) const = default;
		bool operator!=(bound_member_function const&) const = default;

	private:
		void* m_object{};

		using callback_type = ReturnType (*)(void*, ArgTypes...);
		ReturnType (*m_callback)(void*, ArgTypes...);
	};

	template<auto Callback, class ObjectType>
	auto bind_member_function(ObjectType& object)
	{
		return bound_member_function{object, make_type<Callback>{}};
	}
}

#endif