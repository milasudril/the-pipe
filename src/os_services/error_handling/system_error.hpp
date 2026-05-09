#ifndef PIPE_OS_SERVICES_ERROR_HANDLING_SYSTEM_ERROR_HPP
#define PIPE_OS_SERVICES_ERROR_HANDLING_SYSTEM_ERROR_HPP

#include <cstring>
#include <stdexcept>
#include <format>
#include <utility>
#include <cerrno>

namespace Pipe::os_services::error_handling
{
	enum class code{};

	inline auto get_error_code() noexcept
	{ return static_cast<code>(errno); }

	inline auto get_description(code num) noexcept
	{ return strerrordesc_np(static_cast<int>(num)); }

	/**
	 *\brief Exception to be thrown when a syscalls fails in a semi-fatal way
	 */
	class system_error:public std::runtime_error
	{
	public:
		/**
		 * \name Constructs a system_error from a message and an error code using strerrordesc_np
		 */
		//@{
		explicit system_error(char const* msg, code err):
			std::runtime_error{std::format("{}: {}", msg, get_description(err))}
		{}

		explicit system_error(std::string const& msg, code err):
			std::runtime_error{std::format("{}: {}", msg, get_description(err))}
		{}
		//@}
	};
};

#endif