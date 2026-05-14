#ifndef PIPE_COMMON_FWK_ERROR_HANDLER_HPP
#define PIPE_COMMON_FWK_ERROR_HANDLER_HPP

#include <cstddef>
#include <string_view>
#include <utility>
#include <source_location>

namespace Pipe::common_fwk
{
	enum class os_error_code:unsigned int{};


	class error_handler
	{
	public:
		struct context
		{
			std::source_location where{};
			std::string_view what_failed{};
		};

		virtual void raise_error(std::string_view message, context const& ctxt) = 0;
		virtual void raise_system_error(std::string_view message, os_error_code ec, context const& ctxt) = 0;
		virtual void raise_byte_size_computation_error(size_t elem_count, context const& ctxt) = 0;
		virtual void raise_memory_allocation_error(size_t byte_count, context const& ctxt) = 0;
		virtual ~error_handler() = default;
	};
}

#endif