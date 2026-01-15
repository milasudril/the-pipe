#ifndef PROG_IPC_UNIX_DOMAIN_SOCKET_HPP
#define PROG_IPC_UNIX_DOMAIN_SOCKET_HPP

#include "./socket.hpp"

#include <sys/socket.h>
#include <sys/un.h>

namespace prog::ipc
{
	/**
	 * \brief Defines the domain type for sockaddr_un
	 */
	template<>
	struct domain_for_sockaddr<sockaddr_un>
	{
		static constexpr auto domain = AF_UNIX;
	};

	/**
	 * \brief Creates an abstract sockaddr_un
	 */
	inline sockaddr_un make_abstract_sockaddr_un(std::string_view path)
	{
		sockaddr_un ret{};
		ret.sun_family = AF_UNIX;
		if(std::size(path) >= std::size(ret.sun_path) - 2)
		{ throw std::runtime_error{"Address to long"}; }

		memcpy(ret.sun_path + 1, std::data(path), std::size(path));
		return ret;
	}

}
#endif