#ifndef PROG_IPC_SOCKET_HPP
#define PROG_IPC_SOCKET_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <sys/socket.h>

namespace prog::os_services::ipc
{
	/**
	 * \brief Trait for deriving the domain number, given AddressType
	 *
	 * A specialization of this trait should include a static constexpr data member called domain,
	 * which for sockaddr_in would be set to AF_INET.
	 */
	template<class AddressType>
	struct sockaddr_to_domain{};

	/**
	 * \brief A helper to lookup the domain number, given AddressType
	 */
	template<class AddressType>
	inline constexpr auto domain_v = sockaddr_to_domain<AddressType>::domain;

	/**
	 * \brief A tag type used to identify a socket
	 */
	template<auto SocketType, class AddressType>
	struct basic_socket_tag
	{};

	/**
	 * \brief A reference to a basic socket
	 */
	template<auto SocketType, class AddressType>
	using basic_socket_ref = fd::tagged_file_descriptor_ref<basic_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief An owner of a basic socket
	 */
	template<auto SocketType, class AddressType>
	using basic_socket = fd::tagged_file_descriptor<basic_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief Creates a basic socket
	 */
	template<auto SocketType, class AddressType>
	auto make_socket()
	{
		basic_socket<SocketType, AddressType> ret{::socket(domain_v<AddressType>, SocketType, 0)};
		if(ret == nullptr)
		{ throw error_handling::system_error{"Failed to create a socket", errno}; }
		return ret;
	}

	/**
	 * \brief A tag type used to identify a server socket
	 */
	template<auto SocketType, class AddressType>
	struct server_socket_tag
	{};

	/**
	 * \brief A reference to a server socket
	 */
	template<auto SocketType, class AddressType>
	using server_socket_ref = fd::tagged_file_descriptor_ref<server_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief An owner of a server socket
	 */
	template<auto SocketType, class AddressType>
	using server_socket = fd::tagged_file_descriptor<server_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief A Tag type used to identify a connected socket
	 */
	template<auto SocketType, class AddressType>
	struct connected_socket_tag
	{};

	/**
	 * \brief A reference to a connected socket
	 */
	template<auto SocketType, class AddressType>
	using connected_socket_ref = fd::tagged_file_descriptor_ref<connected_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief A reference to a connected socket
	 */
	template<auto SocketType, class AddressType>
	using connected_socket_ref = fd::tagged_file_descriptor_ref<connected_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief An owner of a connected socket
	 */
	template<auto SocketType, class AddressType>
	using connected_socket = fd::tagged_file_descriptor<connected_socket_tag<SocketType, AddressType>>;

	/**
	 * \brief Accepts an incoming connection on server_socket
	 */
	template<auto SocketType, class AddressType>
	connected_socket<SocketType, AddressType> accept(server_socket_ref<SocketType, AddressType> server_socket)
	{
		connected_socket<SocketType, AddressType> ret{::accept(server_socket.native_handle(), nullptr, nullptr)};
		if(ret == nullptr)
		{ throw error_handling::system_error{"Failed to accept connection from socket", errno}; }
		return ret;
	}

	/**
	 * \brief Binds socket to listening address so it becomes a server socket
	 */
	template<auto SocketType, class AddressType>
	server_socket_ref<SocketType, AddressType> bind_and_listen(
		basic_socket_ref<SocketType, AddressType> socket,
		AddressType const& listening_address,
		int connection_backlog
	)
	{
		auto const bind_result = ::bind(
			socket.native_handle(),
			reinterpret_cast<sockaddr const*>(&listening_address),
			sizeof(listening_address)
		);
		if(bind_result == -1)
		{ throw error_handling::system_error{"Failed to bind socket", errno}; }

		auto const listen_result = ::listen(socket.native_handle(), connection_backlog);
		if(listen_result == -1)
		{ throw error_handling::system_error{"Failed to enable listening on socket", errno}; }

		return server_socket_ref<SocketType, AddressType>{socket.native_handle()};
	}

	/**
	 * \brief Creates a server socket, that can accept incoming connections
	 */
	template<auto SocketType, class AddressType>
	server_socket<SocketType, AddressType> make_server_socket(
		AddressType const& listening_address,
		int connection_backlog
	)
	{
		auto socket = make_socket<SocketType, AddressType>();
		auto server_socket_ref = bind_and_listen(socket.release(),listening_address, connection_backlog);
		return server_socket<SocketType, AddressType>{server_socket_ref};
	}

	/**
	 * \brief Connects socket to the address given by connect_to
	 */
	template<auto SocketType, class AddressType>
	connected_socket_ref<SocketType, AddressType> connect(
		basic_socket_ref<SocketType, AddressType> socket,
		AddressType const& connect_to
	)
	{
		auto const result = ::connect(
			socket.native_handle(),
			reinterpret_cast<sockaddr const*>(&connect_to),
			sizeof(connect_to)
		);

		if(result == -1)
		{ throw error_handling::system_error{"Failed to connect socket", errno}; }

		return connected_socket_ref<SocketType, AddressType>{socket.native_handle()};
	}

	/**
	 * \brief Creates a connection to the socket at the other end of connect_to
	 */
	template<auto SocketType, class AddressType>
	connected_socket<SocketType, AddressType> make_connection(AddressType const& connect_to)
	{
		auto socket = make_socket<SocketType, AddressType>();
		auto conn_socket = connect(socket.release(), connect_to);

		return connected_socket<SocketType, AddressType>{conn_socket};
	}

	/**
	 * \brief Enum controlling the behaviour of shutdown
	 */
	enum class connection_shutdown_ops{read = SHUT_RD, write = SHUT_WR, read_write = SHUT_RDWR};

	/**
	 * \brief Shuts down all or part of a connection
	 */
	template<auto SocketType, class AddressType>
	void shutdown(connected_socket_ref<SocketType, AddressType> socket, connection_shutdown_ops ops_to_disable)
	{ ::shutdown(socket.native_handle(), static_cast<int>(ops_to_disable)); }
}

template<auto SocketType, class AddressType>
struct prog::os_services::fd::enabled_fd_conversions<prog::os_services::ipc::connected_socket_tag<SocketType, AddressType>>
{
	static consteval void supports(io::input_file_descriptor_tag){};
	static consteval void supports(io::output_file_descriptor_tag){};
};

#endif