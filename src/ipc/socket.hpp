#ifndef PROG_IPC_SOCKET_HPP
#define PROG_IPC_SOCKET_HPP

#include "src/io/io.hpp"
#include "src/utils/system_error.hpp"

#include <sys/socket.h>

namespace prog::ipc
{
	template<class AddressType>
	struct domain_for_sockaddr{};

	template<class AddressType>
	constexpr auto  domain_v = domain_for_sockaddr<AddressType>::domain;

	/**
	 * \brief A tag type used to identify a socket
	 */
	template<class AddressType, auto SocketType>
	struct basic_socket_tag
	{};

	/**
	 * \brief A reference to a basic socket
	 */
	template<class AddressType, auto SocketType>
	using basic_socket_ref = utils::tagged_file_descriptor_ref<basic_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief An owner of a basic socket
	 */
	template<class AddressType, auto SocketType>
	using basic_socket = utils::tagged_file_descriptor<basic_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief Creates a basic socket
	 */
	template<class AddressType, auto SocketType>
	auto make_socket()
	{
		basic_socket<AddressType, SocketType> ret{::socket(domain_v<AddressType>, SocketType, 0)};
		if(ret == nullptr)
		{ throw utils::system_error{"Failed to create a socket", errno}; }
		return ret;
	}

	/**
	 * \brief A tag type used to identify a server socket
	 */
	template<class AddressType, auto SocketType>
	struct server_socket_tag
	{};

	/**
	 * \brief A reference to a server socket
	 */
	template<class AddressType, auto SocketType>
	using server_socket_ref = utils::tagged_file_descriptor_ref<server_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief An owner of a server socket
	 */
	template<class AddressType, auto SocketType>
	using server_socket = utils::tagged_file_descriptor<server_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief A Tag type used to identify a connected socket
	 */
	template<class AddressType, auto SocketType>
	struct connected_socket_tag
	{};

	/**
	 * \brief A reference to a connected socket
	 */
	template<class AddressType, auto SocketType>
	using connected_socket_ref = utils::tagged_file_descriptor_ref<connected_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief A reference to a connected socket
	 */
	template<class AddressType, auto SocketType>
	using connected_socket_ref = utils::tagged_file_descriptor_ref<connected_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief An owner of a connected socket
	 */
	template<class AddressType, auto SocketType>
	using connected_socket = utils::tagged_file_descriptor<connected_socket_tag<AddressType, SocketType>>;

	/**
	 * \brief Accepts an incoming connection on server_socket
	 */
	template<class AddressType, auto SocketType>
	connected_socket<AddressType, SocketType> accept(server_socket_ref<AddressType, SocketType> server_socket)
	{
		connected_socket<AddressType, SocketType> ret{::accept(server_socket.native_handle(), nullptr, nullptr)};
		if(ret == nullptr)
		{ throw utils::system_error{"Failed to accept connection from socket", errno}; }
		return ret;
	}

	/**
	 * \brief Binds socket to listening address so it becomes a server socket
	 */
	template<auto SocketType, class AddressType>
	server_socket_ref<AddressType, SocketType> bind_and_listen(
		basic_socket_ref<AddressType, SocketType> socket,
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
		{ throw utils::system_error{"Failed to bind socket", errno}; }

		auto const listen_result = ::listen(socket.native_handle(), connection_backlog);
		if(listen_result == -1)
		{ throw utils::system_error{"Failed to enable listening on socket", errno}; }

		return server_socket_ref<AddressType, SocketType>{socket.native_handle()};
	}

	/**
	 * \brief Creates a server socket, that can accept incoming connections
	 */
	template<auto SocketType, class AddressType>
	server_socket<AddressType, SocketType> make_server_socket(
		AddressType const& listening_address,
		int connection_backlog
	)
	{
		auto socket = make_socket<AddressType, SocketType>();
		auto server_socket_ref = bind_and_listen(socket.release(),listening_address, connection_backlog);
		return server_socket<AddressType, SocketType>{server_socket_ref};
	}

	/**
	 * \brief Connects socket to the address given by connect_to
	 */
	template<auto SocketType, class AddressType>
	connected_socket_ref<AddressType, SocketType> connect(
		basic_socket_ref<AddressType, SocketType> socket,
		AddressType const& connect_to
	)
	{
		auto const result = ::connect(
			socket.native_handle(),
			reinterpret_cast<sockaddr const*>(&connect_to),
			sizeof(connect_to)
		);

		if(result == -1)
		{ throw utils::system_error{"Failed to connect socket", errno}; }

		return connected_socket_ref<AddressType, SocketType>{socket.native_handle()};
	}

	/**
	 * \brief Creates a connection to the socket at the other end of connect_to
	 */
	template<auto SocketType, class AddressType>
	connected_socket<AddressType, SocketType> make_connection(AddressType const& connect_to)
	{
		auto socket = make_socket<AddressType, SocketType>();
		auto conn_socket = connect(socket.release(), connect_to);

		return connected_socket<AddressType, SocketType>{conn_socket};
	}

	/**
	 * \brief Enum controlling the behaviour of shutdown
	 */
	enum class connection_shutdown_ops{read = SHUT_RD, write = SHUT_WR, read_write = SHUT_RDWR};

	/**
	 * \brief Shuts down all or part of a connection
	 */
	template<class AddressType, auto SocketType>
	void shutdown(connected_socket_ref<AddressType, SocketType> socket, connection_shutdown_ops ops_to_disable)
	{ ::shutdown(socket.native_handle(), static_cast<int>(ops_to_disable)); }
}

template<class AddressType, auto SocketType>
struct prog::utils::enabled_fd_conversions<prog::ipc::connected_socket_tag<AddressType, SocketType>>
{
	static consteval void supports(io::input_file_descriptor_tag){};
	static consteval void supports(io::output_file_descriptor_tag){};
};

#endif