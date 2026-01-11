#ifndef PROG_UTILS_SYSTEM_ERROR_HPP
#define PROG_UTILS_SYSTEM_ERROR_HPP

#include <cstring>
#include <stdexcept>
#include <format>
#include <utility>

namespace prog::utils
{
	class system_error:public std::runtime_error
	{
	public:
		explicit system_error(char const* msg, int err):
			std::runtime_error{std::format("{}: {}", msg, strerror(err))}
		{}
	};
};

#endif