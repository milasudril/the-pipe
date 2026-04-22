//@	{"target":{"name":"sync_msg_codec.test"}}

#include "./sync_msg_codec.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct request
	{
		int value;
	};

	struct response
	{
		int value;
	};

	struct msg_codec_traits
	{
		using fd_tag = Pipe::os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;
		using incoming_sync_msg_type = std::variant<request>;
		using outgoing_sync_msg_type = std::variant<response>;
		struct client_activity{};
		using sync_fd_activity_event_handler_registred_event =
			Pipe::os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using sync_fd_activity_event = Pipe::os_services::fd::activity_event<client_activity, fd_tag>;
	};

	auto make_sockets()
	{
		Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
		auto const flags = ::fcntl(sockets.socket_b().native_handle(), F_GETFL);
		REQUIRE_NE(flags, -1);
		auto const res = ::fcntl(sockets.socket_b().native_handle(), F_SETFL, O_NONBLOCK);
		REQUIRE_NE(res, -1);
		return sockets;
	}

	struct event_handlers:Pipe::os_services::fd::activity_event_handler_store
	{
		std::optional<Pipe::os_services::fd::event_handler_id> id_to_remove;
		std::optional<Pipe::os_services::fd::activity_status> new_listening_status;

		void remove(Pipe::os_services::fd::event_handler_id id) override
		{
			EXPECT_EQ(id, id_to_remove);
			id_to_remove.reset();
		}

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref,
			Pipe::os_services::fd::activity_status status
		) override
		{
			EXPECT_EQ(status, new_listening_status);
			new_listening_status.reset();
		}

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{
			throw std::runtime_error{"Unexpected function call"};
		}
	};
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_handle_event_fd_activity_event_handler_registered_event)
{
	auto const sockets = make_sockets();

	event_handlers eh_registry;
	Pipe::worker_fwk::sync_msg_codec<msg_codec_traits> codec{65536};
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	{
		auto const is_blocking = ::fcntl(sockets.socket_a().native_handle(), F_GETFL);
		REQUIRE_NE(is_blocking, -1);
		EXPECT_EQ(is_blocking&O_NONBLOCK, O_NONBLOCK);
	}
}
#if 0
TESTCASE(Pipe_worker_fwk_sync_msg_codec_read_and_dispatch_requests_no_bytes_ready_operation_would_have_blocked)
{
	auto const sockets = make_sockets();

	Pipe::worker_fwk::sync_msg_codec<msg_codec_traits> codec{65536};
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_registred_event{
			.fd = sockets.socket_a()
		}
	);
}
#endif