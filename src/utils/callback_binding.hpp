#ifndef PIPE_UTILS_CALLBACK_BINDING_HPP
#define PIPE_UTILS_CALLBACK_BINDING_HPP

#include <bit>

namespace Pipe::utils
{
	template<class ReturnType, class ... ArgTypes>
	class callback_binding
	{
	public:
		callback_binding() = default;

		template<class ObjectType>
		explicit callback_binding(ObjectType& object, ReturnType (*callback)(ObjectType&, ArgTypes...)):
			m_object{&object},
			m_callback{std::bit_cast<callback_type>(callback)}
		{}

		ReturnType operator()(ArgTypes... args) const
		{ return m_callback(m_object, args...); }

	private:
		void* m_object{};

		using callback_type = ReturnType (*)(void*, ArgTypes...);
		callback_type m_callback{};
	};
}

#endif