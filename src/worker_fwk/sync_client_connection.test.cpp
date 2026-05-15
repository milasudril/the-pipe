//@	{"target":{"name":"sync_client_connection.test"}}

#include "./sync_client_connection.hpp"
#include "./port_activity_subscription.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include "src/utils/utils.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <cstdio>
#include <iterator>
#include <limits>
#include <source_location>
#include <string_view>
#include <sys/socket.h>
#include <testfwk/testfwk.hpp>
#include <dlfcn.h>

namespace
{
	struct my_port_activity_subscriber_registry
	{
		std::optional<std::string> expected_server_portname;
		bool fail_port_activity_subscription = false;

		std::optional<Pipe::worker_fwk::port_id> expected_port_id;
		std::optional<Pipe::worker_fwk::port_id> expected_port_id_ready;

		Pipe::worker_fwk::port_id add_port_activity_subscription(
			std::string const& server_portname,
			Pipe::worker_fwk::port_activity_subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{
			EXPECT_EQ(server_portname, expected_server_portname);
			expected_server_portname.reset();
			if(fail_port_activity_subscription)
			{
				fail_port_activity_subscription = false;
				throw std::runtime_error{"Failed to add port activity subscription"};
			}
			return Pipe::worker_fwk::port_id{54};
		}

		void remove_port_activity_subscription(
			Pipe::worker_fwk::port_id port_id,
			Pipe::worker_fwk::port_activity_subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{
			EXPECT_EQ(port_id, expected_port_id);
			expected_port_id.reset();
		}

		void notify_client_ready(Pipe::worker_fwk::port_id port_id)
		{
			EXPECT_EQ(port_id, expected_port_id_ready);
			expected_port_id_ready.reset();
		}
	};

	size_t fail_malloc = std::numeric_limits<size_t>::max();
	size_t malloc_count = 0;

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

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor fd,
			Pipe::os_services::fd::activity_status activity_status
		) override
		{
			REQUIRE_EQ(expected_do_add_call.has_value(), true);
			expected_do_add_call->registred_fd = std::move(fd);
			EXPECT_EQ(expected_do_add_call->initial_listening_status, activity_status);
			return expected_do_add_call->retval;
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

extern "C"
{
	void* malloc(size_t num_bytes)
	{
		if(malloc_count == fail_malloc)
		{
			malloc_count = 0;
			fail_malloc = std::numeric_limits<size_t>::max();
			return nullptr;
		}

		++malloc_count;
		auto real_malloc = reinterpret_cast<void* (*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
		return real_malloc(num_bytes);
	}
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_subscription_request_fail_to_add_subscriber)
{
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	try
	{
		registry.expected_server_portname = "Foobar";
		registry.fail_port_activity_subscription = true;
		conn.handle_request(
			Pipe::worker_sync::port_activity_subscription_request{
				.server_portname = "Foobar"
			},
			Pipe::worker_sync::transaction_id{325},
			ec
		);
		abort();
	}
	catch(...)
	{}
	EXPECT_EQ(conn.num_messages_to_send(), 0);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_subscription_request_fail_to_insert_subscription)
{
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;
	malloc_count = 0;
	fail_malloc = 0; // No additional allocation due to SBO for std::string
	try
	{
		registry.expected_server_portname = "Foobar";
		registry.expected_port_id = Pipe::worker_fwk::port_id{54};
		conn.handle_request(
			Pipe::worker_sync::port_activity_subscription_request{
				.server_portname = "Foobar"
			},
			Pipe::worker_sync::transaction_id{325},
			ec
		);
		abort();
	}
	catch(...)
	{ }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
	EXPECT_EQ(registry.expected_port_id.has_value(), false);
	EXPECT_EQ(conn.num_messages_to_send(), 0);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_subscription_request_succeeds)
{
	my_event_handler_registry eh_registry;
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	registry.expected_server_portname = "Foobar";
	registry.expected_port_id = Pipe::worker_fwk::port_id{54};
	conn.handle_request(
		Pipe::worker_sync::port_activity_subscription_request{
			.server_portname = "Foobar"
		},
		Pipe::worker_sync::transaction_id{325},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(registry.expected_port_id.has_value(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		Pipe::worker_fwk::sync_client_connection::sync_fd_activity_event_handler_registred_event{
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
	EXPECT_EQ(send_result, Pipe::worker_fwk::sync_client_connection::io_status::ok);

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
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{0});
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_unsubscription_subscription_not_found)
{
	my_event_handler_registry eh_registry;
	my_port_activity_subscriber_registry registry;

	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	conn.handle_request(
		Pipe::worker_sync::port_activity_unsubscription{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{126},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(registry.expected_port_id.has_value(), false);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		Pipe::worker_fwk::sync_client_connection::sync_fd_activity_event_handler_registred_event{
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
	EXPECT_EQ(send_result, Pipe::worker_fwk::sync_client_connection::io_status::ok);

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
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{435});
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_unsubscription_subscription_found)
{
	my_event_handler_registry eh_registry;
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;

	// Create subscription
	registry.expected_server_portname = "Foobar";
	registry.expected_port_id = Pipe::worker_fwk::port_id{54};
	conn.handle_request(
		Pipe::worker_sync::port_activity_subscription_request{
			.server_portname = "Foobar"
		},
		Pipe::worker_sync::transaction_id{125},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(registry.expected_port_id.has_value(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	ec.disable_exception_rethrow();
	// Remove subscription
	conn.handle_request(
		Pipe::worker_sync::port_activity_unsubscription{
			.id = Pipe::worker_sync::port_activity_subscription_id{0}
		},
		Pipe::worker_sync::transaction_id{126},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 2);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		Pipe::worker_fwk::sync_client_connection::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 2);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	auto const send_result = conn.send_pending_messages();
	EXPECT_EQ(send_result, Pipe::worker_fwk::sync_client_connection::io_status::ok);

	{
		auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
		EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{125});
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
		EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{0});
	}

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
		EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{0});
	}
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_notify_data_ready)
{
	my_event_handler_registry eh_registry;
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

	Pipe::worker_sync::exception_controller ec;
	// Create subscription
	registry.expected_server_portname = "Foobar";
	registry.expected_port_id = Pipe::worker_fwk::port_id{54};
	conn.handle_request(
		Pipe::worker_sync::port_activity_subscription_request{
			.server_portname = "Foobar"
		},
		Pipe::worker_sync::transaction_id{125},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
	EXPECT_EQ(registry.expected_port_id.has_value(), true);
	EXPECT_EQ(conn.num_messages_to_send(), 1);

	// Try to notify that client is ready. This should fail since the client is already ready
	ec.disable_exception_rethrow();
	try
	{
		conn.handle_message(
			Pipe::worker_sync::client_ready_event{
				.id = Pipe::worker_sync::port_activity_subscription_id{0}
			}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& msg)
	{
		EXPECT_EQ(msg.what(), std::string_view{"Client is already ready"});
	}
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);

	// Send notification that data is ready
	conn.notify_data_ready(Pipe::worker_sync::port_activity_subscription_id{0});
	EXPECT_EQ(conn.num_messages_to_send(), 2);

	// Now the notification should be accepted
	registry.expected_port_id_ready = Pipe::worker_fwk::port_id{54};
	conn.handle_message(
		Pipe::worker_sync::client_ready_event{
			.id = Pipe::worker_sync::port_activity_subscription_id{0}
		}
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
	EXPECT_EQ(conn.num_messages_to_send(), 2);

	auto const sockets = make_sockets();
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	conn.handle_event(
		Pipe::worker_fwk::sync_client_connection::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(conn.num_messages_to_send(), 2);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	auto const send_result = conn.send_pending_messages();
	EXPECT_EQ(send_result, Pipe::worker_fwk::sync_client_connection::io_status::ok);

	{
		auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
		EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{125});
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
		EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{0});
	}

	{
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
		EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{0});
	}
}