//@	{"target":{"name":"msg_file_subscriber_barrier.test"}}

#include "./msg_file_subscriber_barrier.hpp"
#include "src/utils/bound_member_function.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	class my_handler
	{
	public:
		bool call_expected = false;
		void do_it()
		{
			EXPECT_EQ(call_expected, true);
			call_expected = false;
		}
	};
}

TESTCASE(Pipe_worker_fwk_msg_file_subscriber_barrier_inc_num_subscribers)
{
	my_handler handler;
	Pipe::worker_fwk::msg_file_subscriber_barrier barry{
		Pipe::utils::bind_member_function<&my_handler::do_it>(handler)
	};

	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 1);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 2);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 3);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscriber_barrier_fires_when_all_are_completed)
{
	my_handler handler;
	Pipe::worker_fwk::msg_file_subscriber_barrier barry{
		Pipe::utils::bind_member_function<&my_handler::do_it>(handler)
	};

	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 1);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 2);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 3);

	barry.inc_num_ready_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 1);
	barry.inc_num_ready_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 2);

	handler.call_expected = true;
	barry.inc_num_ready_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscriber_barrier_fires_when_sufficiently_many_subcribers_left)
{
	my_handler handler;
	Pipe::worker_fwk::msg_file_subscriber_barrier barry{
		Pipe::utils::bind_member_function<&my_handler::do_it>(handler)
	};

	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 1);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 2);
	barry.inc_num_subscribers();
	EXPECT_EQ(barry.get_num_subscribers(), 3);

	barry.inc_num_ready_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 1);
	barry.inc_num_ready_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 2);

	handler.call_expected = true;
	barry.dec_num_subscribers();
	EXPECT_EQ(barry.get_num_ready_subscribers(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscriber_barrier_dec_does_not_count_below_zero)
{
	my_handler handler;
	Pipe::worker_fwk::msg_file_subscriber_barrier barry{
		Pipe::utils::bind_member_function<&my_handler::do_it>(handler)
	};

	handler.call_expected = true;
	barry.dec_num_subscribers();
}