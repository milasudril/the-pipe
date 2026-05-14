#ifndef PIPE_COMMON_FWK_ERROR_HANDLER_HPP
#define PIPE_COMMON_FWK_ERROR_HANDLER_HPP

#include <type_traits>
#include <cstddef>
#include <string_view>
#include <utility>

namespace Pipe::common_fwk
{
	enum class os_error_code:unsigned int{};

	template<class Traits>
	class error_handler
	{
	public:
		template<class T>
		using type_info = typename Traits::type_info<T>;

		[[noreturn]] void raise_error(std::string_view message)
		{
			do_raise_error(message);
			std::unreachable();
		}

		[[noreturn]] void raise_system_error(std::string_view message, os_error_code ec)
		{
			do_raise_system_error(message, ec);
			std::unreachable();
		}

		template<class T>
		[[noreturn]] void raise_byte_size_computation_error(std::type_identity<T>, size_t elem_count)
		{
			do_raise_byte_size_computation_error(type_info<T>::name, elem_count);
			std::unreachable();
		}

		template<class T>
		[[noreturn]] void raise_memory_allocation_error(std::type_identity<T>,  size_t byte_count)
		{
			do_raise_memory_allocation_error(type_info<T>::name, byte_count);
			std::unreachable();
		}

	protected:
		virtual ~error_handler() = default;
		[[noreturn]] virtual void do_raise_byte_size_computation_error(std::string_view type_name, size_t elem_count) = 0;
		[[noreturn]] virtual void do_raise_memory_allocation_error(std::string_view type_name, size_t byte_count) = 0;
		[[noreturn]] virtual void do_raise_error(std::string_view message) = 0;
		[[noreturn]] virtual void do_raise_system_error(std::string_view message, os_error_code ec) = 0;
	};
}

#endif