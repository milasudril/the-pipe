#ifndef PROG_UTILS_SYSTEM_ERROR_HPP
#define PROG_UTILS_SYSTEM_ERROR_HPP

#include <cstring>
#include <stdexcept>
#include <format>
#include <utility>

namespace prog::error_handling
{
	/**
	 *\brief Exception to be thrown when a syscalls fails in a semi-fatal way
	 */
	class system_error:public std::runtime_error
	{
	public:
		/**
		 * \name Constructs a system_error from a message and an error code using strerror
		 */
		//@{
		explicit system_error(char const* msg, int err):
			std::runtime_error{std::format("{}: {}", msg, strerror(err))}
		{}

		explicit system_error(std::string const& msg, int err):
			std::runtime_error{std::format("{}: {}", msg, strerror(err))}
		{}
		//@}
	};
};

#endif