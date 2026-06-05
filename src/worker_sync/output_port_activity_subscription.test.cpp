//@	{"target":{"name":"output_port_activity_subscription.test"}}

#include "./output_port_activity_subscription.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct output_port_activity_subscriber
	{
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_subscription_id;

		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(expected_subscription_id, id);
			expected_subscription_id.reset();
		}

		~output_port_activity_subscriber()
		{ EXPECT_EQ(expected_subscription_id.has_value(), false); }
	};

	struct output_port
	{
		using subscription_type =
			Pipe::worker_sync::output_port_activity_subscription<
				output_port,
				std::reference_wrapper<output_port_activity_subscriber>
			>;

		std::optional<subscription_type*> added_subscription;

		void add_subscription(subscription_type* subscription)
		{ added_subscription = subscription; }

		void remove_subscription(subscription_type* subscription)
		{
			EXPECT_EQ(added_subscription, subscription);
			added_subscription.reset();
		}


		std::optional<subscription_type*> expected_subscription_decrement;
		void dec_num_busy_subscribers(subscription_type* subscription)
		{
			EXPECT_EQ(expected_subscription_decrement, subscription);
			expected_subscription_decrement.reset();
		}

		~output_port()
		{
			EXPECT_EQ(added_subscription.has_value(), false);
			EXPECT_EQ(expected_subscription_decrement.has_value(), false);
		}
	};
}

TESTCASE(Pipe_worker_sync_ouptut_port_activity_subscription_do_stuff)
{
	Pipe::worker_sync::port_activity_subscription_id id{456};
	output_port my_output_port;
	output_port_activity_subscriber my_output_port_activity_subscriber;

	Pipe::worker_sync::output_port_activity_subscription subscription{
		id,
		&my_output_port,
		std::ref(my_output_port_activity_subscriber)
	};
	EXPECT_EQ(my_output_port.added_subscription, &subscription);

	my_output_port_activity_subscriber.expected_subscription_id = id;
	subscription.notify_data_ready();

	my_output_port.expected_subscription_decrement = &subscription;
	subscription.notify_client_ready();
}