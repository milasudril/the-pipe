#ifndef PROG_IPC_SOCKET_HPP
#define PROG_IPC_SOCKET_HPP

#include "src/io/io.hpp"
#include "src/utils/system_error.hpp"

#include <sys/socket.h>

namespace prog::ipc
{
	/**
	 * \brief A tag type used to identify a socket
	 */
	template<auto Domain, auto Type>
	struct basic_socket_tag
	{};

	/**
	 * \brief A reference to a basic socket
	 */
	template<auto Domain, auto Type>
	using basic_socket_ref = utils::tagged_file_descriptor_ref<basic_socket_tag<Domain, Type>>;

	/**
	 * \brief An owner of a basic socket
	 */
	template<auto Domain, auto Type>
	using basic_socket = utils::tagged_file_descriptor<basic_socket_tag<Domain, Type>>;

	/**
	 * \brief Creates a basic socket
	 */
	template<auto Domain, auto Type>
	auto make_socket()
	{
		basic_socket<Domain, Type> ret{::socket(Domain, Type, 0)};
		if(ret == nullptr)
		{ throw utils::system_error{"Failed to create a socket", errno}; }
		return ret;
	}

	/**
	 * \brief A tag type used to identify a server socket
	 */
	template<auto Domain, auto Type>
	struct server_socket_tag
	{};

	/**
	 * \brief A reference to a basic socket
	 */
	template<auto Domain, auto Type>
	using server_socket_ref = utils::tagged_file_descriptor_ref<server_socket_tag<Domain, Type>>;

	/**
	 * \brief A Tag type used to identify a connected socket
	 */
	template<auto Domain, auto Type>
	struct connected_socket_tag
	{};

	/**
	 * \brief A reference to a connected socket
	 */
	template<auto Domain, auto Type>
	using connected_socket_ref = utils::tagged_file_descriptor_ref<connected_socket_tag<Domain, Type>>;

	/**
	 * \brief A reference to a connected socket
	 */
	template<auto Domain, auto Type>
	using connected_socket_ref = utils::tagged_file_descriptor_ref<connected_socket_tag<Domain, Type>>;

	/**
	 * \brief An owner of a connected socket
	 */
	template<auto Domain, auto Type>
	using connected_socket = utils::tagged_file_descriptor<connected_socket_tag<Domain, Type>>;

	/**
	 * \brief Accepts an incoming connection on server_socket
	 */
	template<auto Domain, auto Type>
	connected_socket<Domain, Type> accept(server_socket_ref<Domain, Type> server_socket)
	{
		connected_socket<Domain, Type> ret{::accept(server_socket.native_handle(), nullptr, nullptr)};
		if(ret == nullptr)
		{ throw utils::system_error{"Failed to accept connection from socket", errno}; }
		return ret;
	}

	/**
	 * \brief Type used for storing a socket address. To be specialized for different domains.
	 */
	template<auto Domain>
	struct sockaddr_for_domain{};

	/**
	 * \brief Helper alias to find the correct sockaddr type
	 */
	template<auto Domain>
	using sockaddr_t = typename sockaddr_for_domain<Domain>::type;

	/**
	 * \brief Binds socket to listening address so it becomes a server socket
	 */
	template<auto Domain, auto Type>
	server_socket_ref<Domain, Type> bind(
		basic_socket_ref<Domain, Type> socket,
		sockaddr_t<Domain> const& listening_address
	)
	{
		auto const result = ::bind(
			socket.native_handle(),
			reinterpret_cast<sockaddr const*>(&listening_address),
			sizeof(listening_address)
		);

		if(result == -1)
		{ throw utils::system_error{"Failed to bind socket", errno}; }
		return server_socket_ref<Domain, Type>{socket.native_handle()};
	}

	/**
	 * \brief Connects socket to the address given by connect_to
	 */
	template<auto Domain, auto Type>
	connected_socket_ref<Domain, Type> connect(basic_socket_ref<Domain, Type> socket, sockaddr_t<Domain> const& connect_to)
	{
		auto const result = ::connect(
			socket.native_handle(),
			reinterpret_cast<sockaddr const*>(&connect_to),
			sizeof(connect_to)
		);

		if(result == -1)
		{ throw utils::system_error{"Failed to connect socket", errno}; }

		return connected_socket_ref<Domain, Type>{socket.native_handle()};
	}

	/**
	 * \brief Makes server_socket listening with a certain backlog
	 */
	template<auto Domain, auto Type>
	void listen(server_socket_ref<Domain, Type> server_socket, int backlog)
	{
		auto const result = ::listen(server_socket.native_handle(), backlog);
		if(result == -1)
		{ throw utils::system_error{"Failed to enable listening on socket", errno}; }
	}

	/**
	 * \brief Enum controlling the behaviour of shutdown
	 */
	enum class shutdown_ops{read = SHUT_RD, write = SHUT_WR, read_write = SHUT_RDWR};

	/**
	 * \brief Shuts down all or part of a connection
	 */
	template<auto Domain, auto Type>
	void shutdown(connected_socket_ref<Domain, Type> socket, shutdown_ops ops_to_disable)
	{ ::shutdown(socket.native_handle(), static_cast<int>(ops_to_disable)); }
}

#endif