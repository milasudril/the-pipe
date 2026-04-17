//@	{"target":{"name":"sync_client_connection.test"}}

#include "./sync_server.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/worker_fwk/port_activity_subscription.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include "src/utils/utils.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <cstdio>
#include <iterator>
#include <source_location>
#include <string_view>
#include <sys/socket.h>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_event_handler_registry:Pipe::os_services::fd::activity_event_handler_store
	{
		void remove(Pipe::os_services::fd::event_handler_id id) override
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

	struct my_port_activity_subscriber_registry
	{
		std::optional<std::string> expected_server_portname;
		bool fail_port_activity_subscription = false;

		std::optional<Pipe::worker_fwk::port_id> expected_port_id;

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
		}

		void notify_client_ready(Pipe::worker_fwk::port_id)
		{}
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
	MsgType recive_message(Pipe::os_services::io::input_file_descriptor_ref fd)
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

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_event_handler_registred_event)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();

	{
		auto const is_blocking = ::fcntl(sockets.socket_a().native_handle(), F_GETFL);
		REQUIRE_NE(is_blocking, -1);
		EXPECT_EQ(is_blocking&O_NONBLOCK, 0);
	}

	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers,
		}
	);
	{
		auto const is_blocking = ::fcntl(sockets.socket_a().native_handle(), F_GETFL);
		REQUIRE_NE(is_blocking, -1);
		EXPECT_EQ(is_blocking&O_NONBLOCK, O_NONBLOCK);
	}

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_client_activity_event_with_error)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers,
		}
	);

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
		}
	);

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
				|Pipe::os_services::fd::activity_status::read
		}
	);

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
				|Pipe::os_services::fd::activity_status::write
		}
	);

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
				|Pipe::os_services::fd::activity_status::read_or_write
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_activity_event_with_read_no_error)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	static constexpr std::array<std::byte, sizeof(Pipe::worker_sync::msg_header)> junk{
		std::byte{0x4A}, std::byte{0x2F}, std::byte{0xA1}, std::byte{0xB9},
    std::byte{0x0C}, std::byte{0x8E}, std::byte{0x33}, std::byte{0x7D},
    std::byte{0x66}, std::byte{0x12}, std::byte{0x99}, std::byte{0x44},
    std::byte{0xED}, std::byte{0x5B}, std::byte{0x00}, std::byte{0xCC}
	};

	auto check_response = [&](std::source_location src_loc = std::source_location::current()){
		printf("Check response: %s:%d\n", src_loc.file_name(), src_loc.line());
		fflush(stdout);
		std::array<std::byte, 39> bytes_sent{};
		auto const read_res = Pipe::os_services::io::read_full(sockets.socket_b(), bytes_sent);
		REQUIRE_EQ(read_res.operation_would_have_blocked(), false);
		REQUIRE_EQ(read_res.bytes_transferred(), std::size(bytes_sent));
		std::array<std::byte, 1> other_buffer{};
		auto const read_res_2 = Pipe::os_services::io::read_full(sockets.socket_b(), other_buffer);
		EXPECT_EQ(read_res_2.operation_would_have_blocked(), true);
		EXPECT_EQ(read_res_2.bytes_transferred(), 0);

		Pipe::worker_sync::decoder<Pipe::worker_sync::msg_header> header_decoder{};
		auto const res = header_decoder.decode(bytes_sent);
		EXPECT_EQ(res, sizeof(Pipe::worker_sync::msg_header));
		EXPECT_EQ(header_decoder.completed(), true);
		auto const header = header_decoder.get_value();
		REQUIRE_EQ(
			header.msg_id,
			(
				Pipe::utils::variant_index_v<
					Pipe::worker_sync::error_response,
					Pipe::worker_sync::server_to_client_message
				>
			)
		);

		Pipe::worker_sync::decoder<Pipe::worker_sync::error_response> msg_decoder{};
		auto const new_res = msg_decoder.decode(
			std::span{std::begin(bytes_sent) + res, std::end(bytes_sent)}
		);
		EXPECT_EQ(new_res + res, read_res.bytes_transferred());
		EXPECT_EQ(msg_decoder.completed(), true);
		auto const msg = msg_decoder.get_value();
		EXPECT_EQ(msg.message, "Invalid type-id");
	};

	{
		auto const write_res = Pipe::os_services::io::write_full(sockets.socket_b(), junk);
		REQUIRE_EQ(write_res.operation_would_have_blocked(), false);
		REQUIRE_EQ(write_res.bytes_transferred(), sizeof(Pipe::worker_sync::msg_header));

		connection.handle_event(
			Pipe::worker_fwk::sync_client_connection::client_activity_event{
				.status = Pipe::os_services::fd::activity_status::read
			}
		);
		check_response();
	}

	{
		auto const write_res = Pipe::os_services::io::write_full(sockets.socket_b(), junk);
		REQUIRE_EQ(write_res.operation_would_have_blocked(), false);
		REQUIRE_EQ(write_res.bytes_transferred(), sizeof(Pipe::worker_sync::msg_header));

		event_handlers.expected_update_listening_status_call =
			my_event_handler_registry::update_listening_status_call{
				.handle = {},
				.new_status = Pipe::os_services::fd::activity_status::read
			};
		connection.handle_event(
			Pipe::worker_fwk::sync_client_connection::client_activity_event{
				.status = Pipe::os_services::fd::activity_status::read_or_write
			}
		);
		check_response();
	}
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_activity_event_with_write_no_error)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

		auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	 auto test = [&](Pipe::os_services::fd::activity_status event_status) {
		// Pre-fill the output buffer to make sure next write will block
		size_t junk_byte_count = 0;
		while(true)
		{
			std::array<std::byte, 4096> blocks{};
			auto const write_result = Pipe::os_services::io::write_full(sockets.socket_a(), blocks);
			junk_byte_count += write_result.bytes_transferred();
			if(write_result.operation_would_have_blocked())
			{ break; }
		}

		event_handlers.expected_update_listening_status_call =
			my_event_handler_registry::update_listening_status_call{
				.handle = {},
				.new_status = Pipe::os_services::fd::activity_status::read_or_write
			};
		connection.send(
			Pipe::worker_sync::data_ready_event{
				.id = Pipe::worker_sync::port_activity_subscription_id{346}
			},
			Pipe::worker_sync::transaction_id{213}
		);

		// Drain junk
		while(true)
		{
			std::array<std::byte, 4096> blocks{};
			std::span read_into{std::begin(blocks), std::min(junk_byte_count, std::size(blocks))};
			auto const read_result = Pipe::os_services::io::read_full(sockets.socket_b(), read_into);
			junk_byte_count -= read_result.bytes_transferred();
			if(read_result.operation_would_have_blocked() || junk_byte_count == 0)
			{ break; }
		}

		// Handling event status should now send the message
		event_handlers.expected_update_listening_status_call =
		my_event_handler_registry::update_listening_status_call{
			.handle = {},
			.new_status = Pipe::os_services::fd::activity_status::read
		};
		connection.handle_event(
			Pipe::worker_fwk::sync_client_connection::client_activity_event{
				.status = event_status
			}
		);

		{
			auto const msg_header = recive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
			REQUIRE_EQ(
				msg_header.msg_id,
				(
					Pipe::utils::variant_index_v<
						Pipe::worker_sync::data_ready_event,
						Pipe::worker_sync::server_to_client_message
					>
				)
			);
			EXPECT_EQ(msg_header.tx_id, Pipe::worker_sync::transaction_id{213});
		}

		{
			const auto msg = recive_message<Pipe::worker_sync::data_ready_event, 8>(sockets.socket_b());
			EXPECT_EQ(msg.id,Pipe::worker_sync::port_activity_subscription_id{346});
		}
	};

	test(Pipe::os_services::fd::activity_status::write);
	test(Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_read_and_dispatch_requests_no_bytes_to_read_blocking)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	auto const res = connection.read_and_dispatch_requests();
	EXPECT_EQ(res, Pipe::worker_fwk::sync_client_connection::connection_status::ok);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_read_and_dispatch_requests_no_bytes_to_read_closed)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);
	sockets.take_socket_b().reset();

	event_handlers.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	auto const res = connection.read_and_dispatch_requests();
	EXPECT_EQ(res, Pipe::worker_fwk::sync_client_connection::connection_status::closed);
}

TESTCASE(Pipe_worker_fwk_read_and_dispatch_requests_recv_request_in_wrong_state)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();

	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	std::array<std::byte, 16> request_buffer{};
	size_t bytes_written = 0;
	{
		Pipe::worker_sync::encoder encoder{
			Pipe::worker_sync::port_activity_subscription_request{
				.server_portname = "fooobaar"
			}
		};
		bytes_written += encoder.encode(request_buffer);
		REQUIRE_EQ(encoder.completed(), true);
	}
	REQUIRE_EQ(bytes_written, std::size(request_buffer));
	auto const write_result = Pipe::os_services::io::write_full(sockets.socket_b(), request_buffer);
	REQUIRE_EQ(write_result.bytes_transferred(), bytes_written);

	auto const res = connection.read_and_dispatch_requests();
	EXPECT_EQ(res, Pipe::worker_fwk::sync_client_connection::connection_status::ok);

	{
		auto const msg_header = recive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
		REQUIRE_EQ(
			msg_header.msg_id,
			(
				Pipe::utils::variant_index_v<
					Pipe::worker_sync::error_response,
					Pipe::worker_sync::server_to_client_message
				>
			)
		);
	}

	{
		auto const msg = recive_message<Pipe::worker_sync::error_response, 8 + 15>(sockets.socket_b());
		EXPECT_EQ(msg.message, "Invalid type-id");
	}
}

TESTCASE(Pipe_worker_fwk_read_and_dispatch_requests_port_activity_subscription_request_fail_to_add_subscriber)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	auto sockets = make_sockets();
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	std::array<std::byte, 30> request_buffer{};
	size_t bytes_written = 0;
	{
		Pipe::worker_sync::encoder encoder{
			Pipe::worker_sync::msg_header{
				.msg_id = Pipe::utils::variant_index_v<
					Pipe::worker_sync::port_activity_subscription_request,
					Pipe::worker_sync::client_to_server_message
				>,
				.tx_id = Pipe::worker_sync::transaction_id{34}
			}
		};
		bytes_written += encoder.encode(request_buffer);
		REQUIRE_EQ(encoder.completed(), true);
	}

	{
		Pipe::worker_sync::encoder encoder{
			Pipe::worker_sync::port_activity_subscription_request{
				.server_portname = "foobar"
			}
		};
		bytes_written += encoder.encode(
			std::span{std::begin(request_buffer) + bytes_written, std::end(request_buffer)}
		);
		REQUIRE_EQ(encoder.completed(), true);
	}
	REQUIRE_EQ(bytes_written, std::size(request_buffer));
	auto const write_result = Pipe::os_services::io::write_full(sockets.socket_b(), request_buffer);
	REQUIRE_EQ(write_result.bytes_transferred(), bytes_written);

	subscriber_registry.expected_server_portname = "foobar";
	subscriber_registry.fail_port_activity_subscription = true;
	auto const res = connection.read_and_dispatch_requests();
	EXPECT_EQ(res, Pipe::worker_fwk::sync_client_connection::connection_status::ok);

	{
		auto const header = recive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
		REQUIRE_EQ(
			header.msg_id,
			(
				Pipe::utils::variant_index_v<
					Pipe::worker_sync::error_response,
					Pipe::worker_sync::server_to_client_message
				>
			)
		);
		EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{34});
	}

	{
		auto const msg =
		recive_message<Pipe::worker_sync::error_response, 40 + 8>(sockets.socket_b());
		EXPECT_EQ(msg.message, "Failed to add port activity subscription");
	}
}