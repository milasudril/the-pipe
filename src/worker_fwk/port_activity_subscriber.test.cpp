//@	{"target":{"name":"port_activity_subscriber.test"}}

#include "./port_activity_subscriber.hpp"

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
