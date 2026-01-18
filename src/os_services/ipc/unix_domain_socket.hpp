#ifndef PROG_IPC_UNIX_DOMAIN_SOCKET_HPP
#define PROG_IPC_UNIX_DOMAIN_SOCKET_HPP

#include "./socket.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/socket.h>
#include <sys/un.h>

namespace prog::os_services::ipc
{
	/**
	 * \brief Defines the domain type for sockaddr_un
	 */
	template<>
	struct sockaddr_to_domain<sockaddr_un>
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

	template<auto SocketType>
	inline ucred get_peer_credentials(connected_socket_ref<SocketType, sockaddr_un> socket)
	{
		ucred ret{};
		socklen_t length = sizeof(ret);
		auto const result = ::getsockopt(socket.native_handle(), SOL_SOCKET, SO_PEERCRED, &ret, &length);
		if(result == -1)
		{ throw error_handling::system_error{"Failed to get peer credentials", errno}; }
		return ret;
	}

}
#endif