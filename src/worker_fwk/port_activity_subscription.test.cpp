//@	{"target":{"name":"port_activity_subscription.test"}}

#include "./port_activity_subscription.hpp"
#include "src/worker_sync/worker_sync.hpp"

#include <optional>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_fwk_port_id_test)
{
	Pipe::worker_fwk::port_id id{};
	EXPECT_EQ(id.value(), 0);
	auto const id_copy = id;
	auto const next = id.next();
	EXPECT_EQ(next, id_copy);
	EXPECT_EQ(id.value(), next.value() + 1);
}

namespace
{
	struct port_activity_subscriber
	{
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_subscription_id;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(id, expected_subscription_id);
			expected_subscription_id.reset();
		}
	};
}

TESTCASE(Pipe_worker_fwk_port_activity_subscriber_ref_equality)
{
	port_activity_subscriber my_subscriber_a;
	port_activity_subscriber my_subscriber_b;
	Pipe::worker_fwk::port_activity_subscriber_ref subscriber_ref_a{my_subscriber_a};
	Pipe::worker_fwk::port_activity_subscriber_ref subscriber_ref_b{my_subscriber_b};
	auto const subscriber_ref_c = subscriber_ref_a;
	EXPECT_NE(subscriber_ref_a, subscriber_ref_b);
	EXPECT_EQ(subscriber_ref_a, subscriber_ref_c);
}

TESTCASE(Pipe_worker_fwk_port_activity_subscriber_notify_data_ready)
{
	port_activity_subscriber my_subscriber;
	Pipe::worker_fwk::port_activity_subscriber_ref subscriber_ref{my_subscriber};
	my_subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{435};
	subscriber_ref.notify_data_ready(Pipe::worker_sync::port_activity_subscription_id{435});
}

namespace
{
	struct port_activity_subscriber_registry
	{
		std::optional<Pipe::worker_fwk::port_id> expected_port_id;
		std::optional<std::string_view> expected_port_name;
		std::optional<Pipe::worker_fwk::port_activity_subscriber_ref> expected_subscriber_ref;
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_subscription_id;

		void notify_client_ready(Pipe::worker_fwk::port_id port_id)
		{
			EXPECT_EQ(port_id, expected_port_id);
			expected_port_id.reset();
		}


		Pipe::worker_fwk::port_id add_port_activity_subscription(
			Pipe::worker_sync::string_type const& port_name,
			Pipe::worker_fwk::port_activity_subscriber_ref subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id subscription_id
		)
		{
			EXPECT_EQ(port_name, expected_port_name);
			EXPECT_EQ(subscriber_ref, expected_subscriber_ref);
			EXPECT_EQ(subscription_id, expected_subscription_id);
			expected_port_name.reset();
			expected_subscriber_ref.reset();
			expected_subscription_id.reset();
			return Pipe::worker_fwk::port_id{4356};
		}

		void remove_port_activity_subscription(
			Pipe::worker_fwk::port_id port_id,
			Pipe::worker_fwk::port_activity_subscriber_ref subscriber_ref,
			Pipe::worker_sync::port_activity_subscription_id subscription_id
		)
		{
			EXPECT_EQ(port_id, expected_port_id);
			EXPECT_EQ(subscriber_ref, expected_subscriber_ref);
			EXPECT_EQ(subscription_id, expected_subscription_id);
			expected_port_id.reset();
			expected_subscriber_ref.reset();
			expected_subscription_id.reset();
		}
	};
}

TESTCASE(port_activity_subscriber_registry_ref_test_callbacks)
{
	port_activity_subscriber_registry my_registry;
	Pipe::worker_fwk::port_activity_subscriber_registry_ref reg_ref{my_registry};

	my_registry.expected_port_id = Pipe::worker_fwk::port_id{435};
	reg_ref.notify_client_ready(Pipe::worker_fwk::port_id{435});

	port_activity_subscriber my_subscriber;
	my_registry.expected_port_name = "foobar";
	my_registry.expected_subscriber_ref =
	Pipe::worker_fwk::port_activity_subscriber_ref{my_subscriber};
	my_registry.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{545};
	auto const port_id = reg_ref.add_port_activity_subscription(
		"foobar",
		Pipe::worker_fwk::port_activity_subscriber_ref{my_subscriber},
		Pipe::worker_sync::port_activity_subscription_id{545}
	);
	EXPECT_EQ(port_id, Pipe::worker_fwk::port_id{4356});

	my_registry.expected_port_id = Pipe::worker_fwk::port_id{42356};
	my_registry.expected_subscriber_ref = Pipe::worker_fwk::port_activity_subscriber_ref{my_subscriber};
	my_registry.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{546};
	reg_ref.remove_port_activity_subscription(
		Pipe::worker_fwk::port_id{42356},
		Pipe::worker_fwk::port_activity_subscriber_ref{my_subscriber},
		Pipe::worker_sync::port_activity_subscription_id{546}
	);
}
