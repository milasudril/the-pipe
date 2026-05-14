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
#if 0
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
#endif

	struct my_port_activity_subscriber_registry
	{
		std::optional<Pipe::worker_sync::string_type> expected_server_portname;
		bool fail_port_activity_subscription = false;

		std::optional<Pipe::worker_fwk::port_id> expected_port_id;

		Pipe::worker_fwk::port_id add_port_activity_subscription(
			Pipe::worker_sync::string_type const& server_portname,
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

		void notify_client_ready(Pipe::worker_fwk::port_id)
		{}
	};

	size_t fail_malloc = std::numeric_limits<size_t>::max();
	size_t malloc_count = 0;
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

	try
	{
		registry.expected_server_portname = "Foobar";
		registry.fail_port_activity_subscription = true;
		conn.handle_request(
			Pipe::worker_sync::port_activity_subscription_request{
				.server_portname = "Foobar"
			},
			Pipe::worker_sync::transaction_id{325}
		);
		abort();
	}
	catch(...)
	{}
	EXPECT_EQ(conn.num_messages_to_send(), 0);
}

TESTCASE(Pipe_worker_fwk_sync_client_connection_port_activity_subscription_request_fail_to_insert_subscription)
{
	my_port_activity_subscriber_registry registry;
	Pipe::worker_fwk::sync_client_connection conn{
		Pipe::worker_fwk::port_activity_subscriber_registry_ref{registry},
		65536
	};

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
			Pipe::worker_sync::transaction_id{325}
		);
		abort();
	}
	catch(...)
	{ }

	EXPECT_EQ(registry.expected_port_id.has_value(), false);
	EXPECT_EQ(conn.num_messages_to_send(), 0);
}
