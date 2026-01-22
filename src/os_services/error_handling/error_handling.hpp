#ifndef PIPE_OS_SERVICES_ERROR_HANDLING_HPP
#define PIPE_OS_SERVICES_ERROR_HANDLING_HPP

#include <cerrno>
#include <type_traits>

/**
 * \brief Error handling utilities
 */
namespace Pipe::os_services::error_handling
{
	/**
	 * \brief Repeatedly calls func, until it returns a value different from -1, or errno is no
	 *        longer EINTR
	 */
	template<class Callable, class... Args>
	auto do_while_eintr(Callable func, Args... args) noexcept
	{
		errno = EINTR;
		std::invoke_result_t<Callable, Args...> result = -1;
		while(errno == EINTR && result == -1)
		{ result = func(args...); }
		return result;
	}
}

#endif