//@	{"target":{"name":"output_port.test"}}

#include "./output_port.hpp"
#include "src/utils/bound_member_function.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct my_subscription
	{
		bool expect_data_ready = false;

		void notify_data_ready()
		{
			EXPECT_EQ(expect_data_ready, true);
			expect_data_ready = false;
		}

		std::optional<bool> busy_status;
		bool is_busy()
		{
			REQUIRE_EQ(busy_status.has_value(), true);
			auto const ret = *busy_status;
			busy_status.reset();
			return ret;
		}

		~my_subscription()
		{
			EXPECT_EQ(expect_data_ready, false);
			EXPECT_EQ(busy_status.has_value(), false);
		}
	};

	struct my_handler
	{
		bool call_expected = false;
		void do_it()
		{
			EXPECT_EQ(call_expected, true);
			call_expected = false;
		}

		~my_handler()
		{ EXPECT_EQ(call_expected, false); }
	};
}

TESTCASE(Pipe_worker_sync_output_port_add_and_remove_subscriptions)
{
	std::array subscriptions{
		my_subscription{},
		my_subscription{},
		my_subscription{}
	};

	my_handler handler;

	Pipe::worker_sync::output_port<my_subscription*> output_port{
		Pipe::utils::bind_member_function<&my_handler::do_it>(handler)
	};

	// Since there is a subscription, nothing has been submitted and no subscriber is busy
	// expect result to be submitted
	handler.call_expected = true;
	output_port.add_subscription(&subscriptions[1]);

	// Result already submitted. Do not submit it again
	output_port.submit_results_if_ready();

	// Result is still already submitted
	output_port.add_subscription(&subscriptions[0]);

	// Now tell the subscribers that data is ready
	subscriptions[0].expect_data_ready = true;
	subscriptions[1].expect_data_ready = true;
	output_port.notify_data_ready();

	// Does nothing since there are busy subscriber
	output_port.submit_results_if_ready();

	// Try to decrement on a non-existing subscription
	try
	{
		output_port.dec_num_busy_subscribers(&subscriptions[2]);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"The subscription does not exist on this port"}); }

	// Decrement numbers busy subscribers
	output_port.dec_num_busy_subscribers(&subscriptions[0]);

	// Try to decrement again on the same port
	try
	{ output_port.dec_num_busy_subscribers(&subscriptions[0]); }
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"The subscriber has already released this resource"}); }

	// Still nothing
	output_port.submit_results_if_ready();

	// Counter reaches zero which submits the result
	handler.call_expected = true;
	output_port.dec_num_busy_subscribers(&subscriptions[1]);

	// Calling notify_data_ready will make subscriptions busy
	subscriptions[0].expect_data_ready = true;
	subscriptions[1].expect_data_ready = true;
	output_port.notify_data_ready();
	output_port.dec_num_busy_subscribers(&subscriptions[0]);
	handler.call_expected = true;
	output_port.dec_num_busy_subscribers(&subscriptions[1]);

	// Try to remove a non-existing subscription
	try
	{
		output_port.remove_subscription(&subscriptions[2]);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"The subscription does not exist on this port"}); }

	// Removing a ready subscription doesn't call handler
	output_port.remove_subscription(&subscriptions[1]);
	try
	{
		output_port.remove_subscription(&subscriptions[1]);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"The subscription does not exist on this port"}); }

	// Removing a busy subscription will call handler
	subscriptions[0].expect_data_ready = true;
	output_port.notify_data_ready();
	handler.call_expected = true;
	output_port.remove_subscription(&subscriptions[0]);
}
