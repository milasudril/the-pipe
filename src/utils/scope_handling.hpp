#ifndef PIPE_UTILS_SCOPE_HANDLING_HPP
#define PIPE_UTILS_SCOPE_HANDLING_HPP

#include <utility>
#include <optional>

namespace Pipe::utils
{
	template<class Callable>
	class at_scope_exit
	{
	public:
		explicit at_scope_exit(Callable&& func):
			m_func{std::move(func)}
		{}

		at_scope_exit(at_scope_exit const&) = delete;
		at_scope_exit(at_scope_exit&&) = delete;
		at_scope_exit& operator=(at_scope_exit const&) = delete;
		at_scope_exit& operator=(at_scope_exit&&) = delete;

		~at_scope_exit()
		{ m_func(); }

	private:
		Callable m_func;
	};

	template<class Callable>
	class maybe_at_scope_exit
	{
	public:
		explicit maybe_at_scope_exit(Callable&& func):
			m_func{std::move(func)}
		{}

		maybe_at_scope_exit(maybe_at_scope_exit const&) = delete;
		maybe_at_scope_exit(maybe_at_scope_exit&&) = delete;
		maybe_at_scope_exit& operator=(maybe_at_scope_exit const&) = delete;
		maybe_at_scope_exit& operator=(maybe_at_scope_exit&&) = delete;

		~maybe_at_scope_exit()
		{
			if(m_func.has_value())
			{ (*m_func)();  }
		}

		void reset()
		{ m_func.reset(); }

	private:
		std::optional<Callable> m_func;
	};
}
#endif
