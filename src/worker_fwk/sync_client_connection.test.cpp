//@	{"target":{"name":"sync_client_connection.test"}}

#include "./sync_client_connection.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/utils/utils.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <cstdio>
#include <iterator>
#include <limits>
#include <source_location>
#include <string_view>
#include <sys/socket.h>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_output_port_activity_subscription_registry
	{
		std::optional<Pipe::worker_fwk::output_port_activity_subscriber_ref> saved_subscriber_ref;

		std::optional<std::string> expected_server_portname;
		std::optional<Pipe::worker_sync::port_activity_subscription_id> returned_subscription_id;
		Pipe::worker_sync::port_activity_subscription_id add_port_activity_subscription(
			std::string const& server_portname,
			Pipe::worker_fwk::output_port_activity_subscriber_ref subscriber_ref
		)
		{
			EXPECT_EQ(server_portname, expected_server_portname);
			expected_server_portname.reset();
			saved_subscriber_ref = subscriber_ref;
			REQUIRE_EQ(returned_subscription_id.has_value(), true);
			auto const ret = *returned_subscription_id;
			returned_subscription_id.reset();
			return ret;
		}

		std::optional<Pipe::worker_sync::port_activity_subscription_id> remove_subscription_subscription_id;
		std::optional<Pipe::worker_fwk::output_port_activity_subscriber_ref>  remove_subscription_subscriber;

		void remove_port_activity_subscription(
			Pipe::worker_sync::port_activity_subscription_id subscription_id,
			Pipe::worker_fwk::output_port_activity_subscriber_ref subscriber
		)
		{
			EXPECT_EQ(remove_subscription_subscription_id.has_value(), true);
			EXPECT_EQ(remove_subscription_subscriber.has_value(), true);
			EXPECT_EQ(subscription_id, remove_subscription_subscription_id);
			EXPECT_EQ(subscriber, remove_subscription_subscriber);
			remove_subscription_subscription_id.reset();
			remove_subscription_subscriber.reset();
		}

		std::optional<Pipe::worker_sync::port_activity_subscription_id> notify_subscription_subscription_id;
		std::optional<Pipe::worker_fwk::output_port_activity_subscriber_ref>  notify_subscription_subscriber;
		void notify_client_ready(
			Pipe::worker_sync::port_activity_subscription_id subscription_id,
			Pipe::worker_fwk::output_port_activity_subscriber_ref subscriber
		)
		{
			EXPECT_EQ(notify_subscription_subscription_id.has_value(), true);
			EXPECT_EQ(notify_subscription_subscriber.has_value(), true);
			EXPECT_EQ(subscription_id, notify_subscription_subscription_id);
			EXPECT_EQ(subscriber, notify_subscription_subscriber);
			notify_subscription_subscription_id.reset();
			notify_subscription_subscriber.reset();
		}

		std::optional<Pipe::worker_fwk::output_port_activity_subscriber_ref> remove_subscriber_subscriber;
		void remove_output_port_activity_subscriber(Pipe::worker_fwk::output_port_activity_subscriber_ref subscriber)
		{
			EXPECT_EQ(remove_subscriber_subscriber.has_value(), true);
			EXPECT_EQ(subscriber, remove_subscriber_subscriber);
			remove_subscriber_subscriber.reset();
		}

		~my_output_port_activity_subscription_registry()
		{
			EXPECT_EQ(expected_server_portname.has_value(), false);
			EXPECT_EQ(remove_subscription_subscription_id.has_value(), false);
			EXPECT_EQ(remove_subscription_subscriber.has_value(), false);
			EXPECT_EQ(notify_subscription_subscription_id.has_value(), false);
			EXPECT_EQ(notify_subscription_subscriber.has_value(), false);
			EXPECT_EQ(remove_subscriber_subscriber.has_value(), false);
		}
	};

	using sync_client_connection = Pipe::worker_fwk::sync_client_connection<my_output_port_activity_subscription_registry>;

	struct my_event_handler_registry:Pipe::os_services::fd::activity_event_handler_store
	{
		void remove(Pipe::os_services::fd::event_handler_id id) noexcept override
		{
			EXPECT_EQ(expected_remove_id.has_value(), true);
			EXPECT_EQ(id, expected_remove_id);
			expected_remove_id.reset();
		}

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref handle,
			Pipe::os_services::fd::activity_status new_status
		) override
		{
			REQUIRE_EQ(expected_update_listening_status_call.has_value(), true);
			EXPECT_EQ(handle.get(), expected_update_listening_status_call->handle.get());
			EXPECT_EQ(new_status, expected_update_listening_status_call->new_status);
			expected_update_listening_status_call.reset();
		}

		std::pair<void*, Pipe::os_services::fd::event_handler_id> do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{
			throw std::runtime_error{"Unexpected call to do_add"};
		}

		std::optional<Pipe::os_services::fd::event_handler_id> expected_remove_id;

		struct update_listening_status_call
		{
			Pipe::os_services::fd::saved_event_handler_ref handle;
			Pipe::os_services::fd::activity_status new_status;
		};
		std::optional<update_listening_status_call> expected_update_listening_status_call;

		struct do_add_call
		{
			Pipe::os_services::fd::activity_status initial_listening_status;
			Pipe::os_services::fd::file_descriptor registred_fd;
			Pipe::os_services::fd::event_handler_id retval;
		};
		std::optional<do_add_call> expected_do_add_call;
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

	template<class MsgType, size_t ExpectedByteCount>
	MsgType receive_message(Pipe::os_services::io::input_file_descriptor_ref fd)
	{
		std::array<std::byte, ExpectedByteCount> buffer;
		auto const read_result = read_full(fd, buffer);
		REQUIRE_EQ(read_result.operation_would_have_blocked(), false);
		REQUIRE_EQ(read_result.bytes_transferred(), ExpectedByteCount);
		Pipe::worker_sync::decoder<MsgType> decoder{};
		auto const res = decoder.decode(buffer);
		EXPECT_EQ(res, ExpectedByteCount);
		EXPECT_EQ(decoder.completed(), true);
		return decoder.get_value();
	}
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_subscription_request)
{
	my_event_handler_registry eh_registry;
	my_output_port_activity_subscription_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		std::ref(registry),
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	registry.expected_server_portname = "Foobar";
	registry.returned_subscription_id = Pipe::worker_sync::port_activity_subscription_id{54};
	conn.handle_request(
		Pipe::worker_sync::port_activity_subscription_request{
			.server_portname = "Foobar"
		},
		Pipe::worker_sync::transaction_id{325},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		sync_client_connection::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	auto const send_result = conn.send_pending_messages();
	EXPECT_EQ(send_result,sync_client_connection::io_status::ok);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(
		header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::port_activity_subscription_response,
				Pipe::worker_sync::server_to_client_message
			>
		)
	);

	auto const body = receive_message<Pipe::worker_sync::port_activity_subscription_response, 8>(
		sockets.socket_b()
	);
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{54});

	registry.remove_subscriber_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_unsubscription)
{
	my_event_handler_registry eh_registry;
	my_output_port_activity_subscription_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		std::ref(registry),
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	// Remove subscription
	registry.remove_subscription_subscription_id = Pipe::worker_sync::port_activity_subscription_id{64365};
	registry.remove_subscription_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
	conn.handle_request(
		Pipe::worker_sync::port_activity_unsubscription{
			.id = Pipe::worker_sync::port_activity_subscription_id{64365}
		},
		Pipe::worker_sync::transaction_id{126},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		sync_client_connection::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	auto const send_result = conn.send_pending_messages();
	EXPECT_EQ(send_result, sync_client_connection::io_status::ok);

	{
		auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
		EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{126});
		EXPECT_EQ(
			header.msg_id,
			(
				Pipe::utils::variant_index_v<
					Pipe::worker_sync::port_activity_unsubscription_response,
					Pipe::worker_sync::server_to_client_message
				>
			)
		);
		auto const body = receive_message<Pipe::worker_sync::port_activity_unsubscription_response, 8>(
			sockets.socket_b()
		);
		EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{64365});
	}

	registry.remove_subscriber_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_client_ready)
{
	my_event_handler_registry eh_registry;
	my_output_port_activity_subscription_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		std::ref(registry),
		65536
	};

	registry.notify_subscription_subscription_id = Pipe::worker_sync::port_activity_subscription_id{54};
	registry.notify_subscription_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
	conn.handle_message(
		Pipe::worker_sync::client_ready_event{
			.id = Pipe::worker_sync::port_activity_subscription_id{54}
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 0);

	registry.remove_subscriber_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_notify_data_ready)
{
	my_event_handler_registry eh_registry;
	my_output_port_activity_subscription_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		std::ref(registry),
		65536
	};

	conn.notify_data_ready(Pipe::worker_sync::port_activity_subscription_id{4365});
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		sync_client_connection::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	auto const send_result = conn.send_pending_messages();
	EXPECT_EQ(send_result, sync_client_connection::io_status::ok);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{});
	EXPECT_EQ(
		header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::data_ready_event,
				Pipe::worker_sync::server_to_client_message
			>
		)
	);

	auto const body = receive_message<Pipe::worker_sync::data_ready_event, 8>(
		sockets.socket_b()
	);
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{4365});

	registry.remove_subscriber_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_error_response)
{
	my_event_handler_registry eh_registry;
	my_output_port_activity_subscription_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		std::ref(registry),
		65536
	};
	Pipe::worker_sync::exception_controller ec;

	try
	{
		conn.handle_response(
			Pipe::worker_sync::error_response{
				.message = "This is the error message"
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"This is the error message"});
	}

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	registry.remove_subscriber_subscriber = Pipe::worker_fwk::output_port_activity_subscriber_ref{conn};
}
