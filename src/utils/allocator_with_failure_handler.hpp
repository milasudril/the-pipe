#ifndef PIPE_UTILS_ALLOCATOR_WITH_FAILURE_HANDLER_HPP
#define PIPE_UTILS_ALLOCATOR_WITH_FAILURE_HANDLER_HPP

#include <new>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace Pipe::utils
{
	template <class T, class FailureHandler>
	class allocator_with_failure_handler
	{
	public:
		using value_type = T;

		template<class Self>
		[[nodiscard]] T* allocate(this Self&& self, std::size_t n)
		{

			auto const max_num_elements = std::numeric_limits<size_t>::max()/sizeof(T);
			if(n > max_num_elements)
			{ throw std::bad_array_new_length{}; }

			auto const num_bytes_to_allocate = n*sizeof(T);
			auto const ret = ::operator new(num_bytes_to_allocate, std::nothrow);
			if(ret == nullptr) [[unlikely]]
			{
				std::forward<FailureHandler>(self.m_failure_handler)
					.memory_allocation_failed(std::type_identity<T>{}, n);
				throw std::bad_alloc{};
			}

			return static_cast<T*>(ret);
		}

		static void deallocate(T* p, std::size_t n) noexcept
		{ ::operator delete(p, n * sizeof(T)); }

		bool operator==(allocator_with_failure_handler const&) const = default;
		bool operator!=(allocator_with_failure_handler const&) const = default;

		template<class Self>
		auto&& failure_handler(this Self&& self) noexcept
		{ return std::forward<FailureHandler>(self.m_failure_handler); }

	private:
		[[no_unique_address]] FailureHandler m_failure_handler;
	};
}
#endif