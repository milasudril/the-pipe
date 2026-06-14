//@	{"target":{"name":"sync_server.test"}}

#include "./sync_server.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"

#include <sys/socket.h>
#include <testfwk/testfwk.hpp>
#include <thread>

namespace
{
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
		) noexcept override
		{
			if(!expected_update_listening_status_call.has_value())
			{ Pipe::log::terminate_with_message("Unexpected update_listening_status"); }

			EXPECT_EQ(handle.get(), expected_update_listening_status_call->handle.get());
			EXPECT_EQ(new_status, expected_update_listening_status_call->new_status);
			expected_update_listening_status_call.reset();
		}

		std::pair<void*, Pipe::os_services::fd::event_handler_id> do_add(
			event_handler_info const& event_handler,
			Pipe::os_services::fd::file_descriptor fd,
			Pipe::os_services::fd::activity_status activity_status
		) override
		{
			REQUIRE_EQ(expected_do_add_call.has_value(), true);
			expected_do_add_call->registred_fd = std::move(fd);
			EXPECT_EQ(expected_do_add_call->initial_listening_status, activity_status);
			return std::pair{event_handler.object_address.address, expected_do_add_call->retval};
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

		Pipe::worker_sync::port_activity_subscription_id add_port_activity_subscription(
			std::string const&,
			Pipe::worker_fwk::port_activity_subscriber_ref
		)
		{
			throw std::runtime_error{"Unexpected call to add_port_activity_subscription"};
		}

		void remove_port_activity_subscription(
			Pipe::worker_sync::port_activity_subscription_id,
			Pipe::worker_fwk::port_activity_subscriber_ref
		)
		{
			throw std::runtime_error{"Unexpected call to remove_port_activity_subscription"};
		}

		void notify_client_ready(
			Pipe::worker_sync::port_activity_subscription_id,
			Pipe::worker_fwk::port_activity_subscriber_ref
		)
		{
			throw std::runtime_error{"Unexpected call to notify_client_ready"};
		}

		void remove_port_activity_subscriber(Pipe::worker_fwk::port_activity_subscriber_ref)
		{}
	};
}

TESTCASE(Pipe_worker_fwk_sync_server_create)
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
		std::ref(port_activity_subscriber_registry)
	);
	auto registered_fd = std::move(event_handlers.expected_do_add_call->registred_fd);

	EXPECT_EQ(std::size(server_info.socket_name), Pipe::os_services::ipc::abstract_sunpath_maxlength);

	EXPECT_EQ(server_info.event_handler.second, Pipe::os_services::fd::event_handler_id{324});

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
	EXPECT_EQ(
		&server_info.event_handler.first.get().get_registry().get(),
		&port_activity_subscriber_registry
	);
}

TESTCASE(Pipe_worker_fwk_sync_server_register_and_accept_connection)
{
	my_event_handler_registry event_handlers;

	auto const socket_name =
		Pipe::utils::random_printable_ascii_string(Pipe::os_services::ipc::abstract_sunpath_maxlength);
	auto const server_socket = Pipe::os_services::ipc::make_server_socket<SOCK_STREAM>(
		Pipe::os_services::ipc::make_abstract_sockaddr_un(socket_name),
		1024
	);

	my_port_activity_subscriber_registry port_activity_subscriber_registry;
	Pipe::worker_fwk::sync_server server{std::ref(port_activity_subscriber_registry)};

	server.handle_event(
		Pipe::worker_fwk::sync_server<std::reference_wrapper<my_port_activity_subscriber_registry>>::activity_event_handler_registered_event{
			.fd = server_socket.get(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &event_handlers
		}
	);

	std::jthread client_thread{
		[socket_name](){
			auto const socket = Pipe::os_services::ipc::make_connection<SOCK_STREAM>(
				Pipe::os_services::ipc::make_abstract_sockaddr_un(socket_name)
			);
		}
	};

	event_handlers.expected_do_add_call = my_event_handler_registry::do_add_call{
		Pipe::os_services::fd::activity_status::read,
		Pipe::os_services::fd::file_descriptor{},
		Pipe::os_services::fd::event_handler_id{324}
	};

	server.handle_event(
		Pipe::worker_fwk::sync_server<std::reference_wrapper<my_port_activity_subscriber_registry>>::server_activity_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);
}
