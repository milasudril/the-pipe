//@	{"target":{"name":"sync_server.test"}}

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
		Pipe::worker_fwk::port_id add_port_activity_subscription(
			std::string_view,
			Pipe::worker_fwk::port_activity_subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{
			return Pipe::worker_fwk::port_id{54};
		}

		void remove_port_activity_subscription(
			Pipe::worker_fwk::port_id,
			Pipe::worker_fwk::port_activity_subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{}

		void notify_client_ready(Pipe::worker_fwk::port_id)
		{}
	};
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_handle_event_handler_registred_event)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
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

	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
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

TESTCASE(Pipe_worker_fw_sync_client_connection_handle_activity_event_with_read_no_error)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	{
		auto const flags = ::fcntl(sockets.socket_b().native_handle(), F_GETFL);
		REQUIRE_NE(flags, -1);
		auto const res = ::fcntl(sockets.socket_b().native_handle(), F_SETFL, O_NONBLOCK);
		REQUIRE_NE(res, -1);
	}

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

TESTCASE(Pipe_worker_fw_sync_client_connection_handle_activity_event_with_write_no_error)
{
	my_port_activity_subscriber_registry subscriber_registry;
	Pipe::worker_fwk::sync_client_connection connection{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{subscriber_registry}
	};

	Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
	my_event_handler_registry event_handlers;
	connection.handle_event(
		Pipe::worker_fwk::sync_client_connection::client_activity_event_handler_registered_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	{
		auto const flags = ::fcntl(sockets.socket_b().native_handle(), F_GETFL);
		REQUIRE_NE(flags, -1);
		auto const res = ::fcntl(sockets.socket_b().native_handle(), F_SETFL, O_NONBLOCK);
		REQUIRE_NE(res, -1);
	}

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

		// TODO: Currently, read and write events are not handled from the same context. This is because
		// read could trigger an error which causes the handler to be deleted
		if(!can_read(event_status))
		{
			event_handlers.expected_update_listening_status_call =
				my_event_handler_registry::update_listening_status_call{
					.handle = {},
					.new_status = Pipe::os_services::fd::activity_status::read
				};
		}
		connection.handle_event(
			Pipe::worker_fwk::sync_client_connection::client_activity_event{
				.status = event_status
			}
		);

		if(can_read(event_status))
		{ return; }

		std::array<std::byte, 24> bytes_written{};
		auto const write_result = Pipe::os_services::io::read_full(sockets.socket_b(), bytes_written);
		EXPECT_EQ(write_result.operation_would_have_blocked(), false);
		EXPECT_EQ(write_result.bytes_transferred(), 24);

		{
			Pipe::worker_sync::decoder<Pipe::worker_sync::msg_header> decoder{};
			auto const res = decoder.decode(bytes_written);
			EXPECT_EQ(res, sizeof(Pipe::worker_sync::msg_header));
			EXPECT_EQ(decoder.completed(), true);
			auto const header = decoder.get_value();
			REQUIRE_EQ(
				header.msg_id,
				(
					Pipe::utils::variant_index_v<
						Pipe::worker_sync::data_ready_event,
						Pipe::worker_sync::server_to_client_message
					>
				)
			);
			EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{213});
		}

		{
			Pipe::worker_sync::decoder<Pipe::worker_sync::data_ready_event> decoder{};
			auto const res = decoder.decode(std::span{ std::begin(bytes_written) + 16, std::end(bytes_written)});
			EXPECT_EQ(res, 8);
			EXPECT_EQ(decoder.completed(), true);
			auto const msg = decoder.get_value();
			EXPECT_EQ(msg.id,Pipe::worker_sync::port_activity_subscription_id{346});
		}
	};

	test(Pipe::os_services::fd::activity_status::write);
	test(Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_server_init)
{
	my_event_handler_registry event_handlers;
	event_handlers.expected_do_add_call = my_event_handler_registry::do_add_call{
		Pipe::os_services::fd::activity_status::read,
		Pipe::os_services::fd::file_descriptor{},
		Pipe::os_services::fd::event_handler_id{324}
	};

	my_port_activity_subscriber_registry port_activity_subscriber_registry;
	auto const server_info = Pipe::worker_fwk::make_sync_server(
		event_handlers,
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{port_activity_subscriber_registry}
	);
	auto registered_fd = std::move(event_handlers.expected_do_add_call->registred_fd);

	EXPECT_EQ(std::size(server_info.socket_name), Pipe::os_services::ipc::abstract_sunpath_maxlength);

	EXPECT_EQ(server_info.event_handler_id, Pipe::os_services::fd::event_handler_id{324});

	REQUIRE_NE(registered_fd.get(), nullptr);
	sockaddr_un addr{};
	socklen_t addr_size = sizeof(addr);
	auto const res = getsockname(
		registered_fd.get().native_handle(),
		reinterpret_cast<sockaddr*>(&addr),
		&addr_size
	);
	REQUIRE_NE(res, -1);
	auto const addr_string = Pipe::os_services::ipc::to_string(addr);
	EXPECT_EQ(std::size(addr_string), std::size(server_info.socket_name) + 1);
	EXPECT_EQ(
		(std::string_view{std::begin(addr_string) + 1, std::end(addr_string)}),
		server_info.socket_name
	);
}
