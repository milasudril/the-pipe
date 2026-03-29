#ifndef PIPE_IPC_UNIX_DOMAIN_SOCKET_HPP
#define PIPE_IPC_UNIX_DOMAIN_SOCKET_HPP

#include "./socket.hpp"
#include "src/os_services/error_handling/system_error.hpp"
#include "src/utils/utils.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <type_traits>

namespace Pipe::os_services::ipc
{
	/**
	 * \brief Defines the domain type for sockaddr_un
	 */
	template<>
	struct sockaddr_to_domain<sockaddr_un>
	{
		static constexpr auto domain = AF_UNIX;
	};

	inline constexpr auto sunpath_maxlength = utils::array_size_v<
		decltype(std::declval<sockaddr_un>().sun_path)
	> - 1;

	inline constexpr auto abstract_sunpath_maxlength = sunpath_maxlength - 1;

	/**
	 * \brief Creates an abstract sockaddr_un
	 */
	inline sockaddr_un make_abstract_sockaddr_un(std::string_view path)
	{
		sockaddr_un ret{};
		ret.sun_family = AF_UNIX;
		if(std::size(path) > abstract_sunpath_maxlength)
		{ throw std::runtime_error{"Address to long"}; }

		memcpy(ret.sun_path + 1, std::data(path), std::size(path));
		return ret;
	}

	inline std::string to_string(sockaddr_un const& addr)
	{
		std::string ret;
		for(size_t k = 0; k != sunpath_maxlength; ++k)
		{ ret += addr.sun_path[k]; }

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