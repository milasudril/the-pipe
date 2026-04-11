//@	{"target":{"name":"sync_server.test"}}

#include "./sync_server.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "testfwk/validation.hpp"

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
		}

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref handle,
			Pipe::os_services::fd::activity_status new_status
		) override
		{
			EXPECT_EQ(expected_update_listening_status_call.has_value(), true);
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

	struct my_output_port_provider
	{
		Pipe::worker_fwk::port_id add_subscriber(std::string_view)
		{
			return Pipe::worker_fwk::port_id{54};
		}

		void remove_subscriber(Pipe::worker_fwk::port_id)
		{}

		void notify_client_ready(Pipe::worker_fwk::port_id)
		{}
	};
}

TESTCASE(Pipe_worker_fwk_sync_server_init)
{
	my_event_handler_registry event_handlers;
	event_handlers.expected_do_add_call = my_event_handler_registry::do_add_call{
		Pipe::os_services::fd::activity_status::read,
		Pipe::os_services::fd::file_descriptor{},
		Pipe::os_services::fd::event_handler_id{324}
	};

	my_output_port_provider output_port_provider;
	auto const server_info = Pipe::worker_fwk::make_sync_server(
		event_handlers,
		Pipe::worker_fwk::output_port_provider_ref{output_port_provider}
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
