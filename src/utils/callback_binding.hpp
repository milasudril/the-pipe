#ifndef PIPE_UTILS_CALLBACK_BINDING_HPP
#define PIPE_UTILS_CALLBACK_BINDING_HPP

#include <bit>

namespace Pipe::utils
{
	template<auto Value>
	struct make_type
	{
		static constexpr auto value = Value;
	};

	template<class ReturnType, class ... ArgTypes>
	class callback_binding
	{
	public:
		callback_binding() = default;

		template<class ObjectType, ReturnType (ObjectType::*Callback)(ArgTypes...)>
		explicit callback_binding(ObjectType& object, make_type<Callback>):
			m_object{&object},
			m_callback{[](void* object, ArgTypes... args){
				return (static_cast<ObjectType*>(object)->*make_type<Callback>::value)(args...);
			}}
		{}

		template<class ObjectType, ReturnType (*Callback)(ObjectType&, ArgTypes...)>
		explicit callback_binding(ObjectType& object, make_type<Callback>):
			m_object{&object},
			m_callback{
				[](void* object, ArgTypes... args){
					return make_type<Callback>::value(*static_cast<ObjectType*>(object), args...);
				}
			}
		{}

		ReturnType operator()(ArgTypes... args) const
		{ return m_callback(m_object, args...); }

	private:
		void* m_object{};

		using callback_type = ReturnType (*)(void*, ArgTypes...);
		ReturnType (*m_callback)(void*, ArgTypes...);
	};

	template<auto Callback, class ObjectType>
	auto make_binding(ObjectType& object)
	{
		return callback_binding{object, make_type<Callback>{}};
	}
}

#endif