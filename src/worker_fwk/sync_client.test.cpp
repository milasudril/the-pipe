//@	{"target":{"name":"sync_client.test"}}

#include "./sync_client.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct input_port_activity_subscriber
	{
		struct subscription_transaction
		{};

		struct unsubscription_transaction
		{};

		std::optional<void const*> expected_conn_lost_ptr;
		void sync_client_lost_connection_to_server(void const* ptr)
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), true);
			EXPECT_EQ(expected_conn_lost_ptr, ptr);
			expected_conn_lost_ptr.reset();
		}

		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_data_ready_id;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(expected_data_ready_id.has_value(), true);
			EXPECT_EQ(expected_data_ready_id, id);
			expected_data_ready_id.reset();
		}

		void subscription_completed(
			Pipe::worker_sync::transaction_id,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{}

		void unsubscription_completed(Pipe::worker_sync::transaction_id)
		{}

		~input_port_activity_subscriber()
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), false);
			EXPECT_EQ(expected_data_ready_id.has_value(), false);
		}
	};

};

TESTCASE(Pipe_worker_fwk_sync_client_construct)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_without_tx_id)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	Pipe::worker_sync::exception_controller ec;
	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The error message"});
	}
	subscriber.expected_conn_lost_ptr = &client;
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_no_ongoing_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	Pipe::worker_sync::exception_controller ec;
	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{4324},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"});
	}
	subscriber.expected_conn_lost_ptr = &client;
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

