#ifndef PIPE_UTILS_ALLOCATOR_WITH_FAILURE_HANDLER_HPP
#define PIPE_UTILS_ALLOCATOR_WITH_FAILURE_HANDLER_HPP

#include "./unwrap.hpp"

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

		allocator_with_failure_handler() = default;
		explicit allocator_with_failure_handler(FailureHandler const& failure_handler):
			m_failure_handler{failure_handler}
		{}

    template<class Other>
    allocator_with_failure_handler(allocator_with_failure_handler<Other, FailureHandler> const& other):
			m_failure_handler{other.failure_handler()}
    {}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			auto const max_num_elements = std::numeric_limits<size_t>::max()/sizeof(T);
			if(n > max_num_elements)
			{
				if(has_value(m_failure_handler))
				{ unwrap(m_failure_handler).raise_byte_size_computation_error(std::type_identity<T>{}, n); }

				throw std::bad_array_new_length{};
			}

			auto const num_bytes_to_allocate = n*sizeof(T);
			auto const ret = ::operator new(num_bytes_to_allocate, std::nothrow);
			if(ret == nullptr) [[unlikely]]
			{
				if(has_value(m_failure_handler))
				{ unwrap(m_failure_handler).raise_memory_allocation_error(std::type_identity<T>{}, n); }
				throw std::bad_alloc{};
			}

			return static_cast<T*>(ret);
		}

		static void deallocate(T* p, std::size_t n) noexcept
		{ ::operator delete(p, n * sizeof(T)); }

		bool operator==(allocator_with_failure_handler const&) const = default;
		bool operator!=(allocator_with_failure_handler const&) const = default;

		auto& failure_handler() noexcept
		{ return m_failure_handler; }

		auto& failure_handler() const noexcept
		{ return m_failure_handler; }

	private:
		[[no_unique_address]] FailureHandler m_failure_handler{};
	};
}
#endif