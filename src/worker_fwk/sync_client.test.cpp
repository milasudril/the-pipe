//@	{"target":{"name":"sync_client.test"}}

#include "./sync_client.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct input_port_activity_subscriber
	{
		struct subscription_transaction
		{
			int value;
			constexpr auto operator<=>(subscription_transaction const&) const = default;
		};

		struct unsubscription_transaction
		{
			int value;
			constexpr auto operator<=>(unsubscription_transaction const&) const = default;
		};

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

		std::optional<subscription_transaction> expected_subscription_transaction;
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_subscription_id;
		void subscription_completed(
			subscription_transaction transaction,
			Pipe::worker_sync::port_activity_subscription_id subscription_id
		)
		{
			EXPECT_EQ(expected_subscription_transaction.has_value(), true);
			EXPECT_EQ(expected_subscription_transaction, transaction);
			expected_subscription_transaction.reset();
			EXPECT_EQ(expected_subscription_id.has_value(), true);
			EXPECT_EQ(subscription_id, expected_subscription_id);
			expected_subscription_id.reset();
		}

		std::optional<unsubscription_transaction> expected_unsubscription_transaction;
		void unsubscription_completed(unsubscription_transaction transaction)
		{
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), true);
			EXPECT_EQ(expected_unsubscription_transaction, transaction);
			expected_unsubscription_transaction.reset();
		}

		~input_port_activity_subscriber()
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), false);
			EXPECT_EQ(expected_data_ready_id.has_value(), false);
			EXPECT_EQ(expected_subscription_transaction.has_value(), false);
			EXPECT_EQ(expected_subscription_id.has_value(), false);
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), false);
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


TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_existing_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"My port",
		input_port_activity_subscriber::subscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{0},
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

TESTCASE(Pipe_worker_fwk_sync_client_handle_data_ready_event)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	subscriber.expected_data_ready_id = Pipe::worker_sync::port_activity_subscription_id{3};
	client.handle_message(
		Pipe::worker_sync::data_ready_event{
			.id = Pipe::worker_sync::port_activity_subscription_id{3}
		}
	);
	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_no_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_subscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_wrong_transaction_type)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.unsubscribe_from_port(
		Pipe::worker_sync::port_activity_subscription_id{435},
		input_port_activity_subscriber::unsubscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_subscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Unexpected transaction type"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"The port",
		input_port_activity_subscriber::subscription_transaction{1}
	);

	subscriber.expected_subscription_transaction = input_port_activity_subscriber::subscription_transaction{1};
	subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{435};
	client.handle_response(
		Pipe::worker_sync::port_activity_subscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{0},
		ec
	);

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);

	subscriber.expected_conn_lost_ptr = &client;
}

////

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response_no_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_unsubscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response_wrong_transaction_type)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"The port",
		input_port_activity_subscriber::subscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_unsubscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Unexpected transaction type"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.unsubscribe_from_port(
		Pipe::worker_sync::port_activity_subscription_id{435},
		input_port_activity_subscriber::unsubscription_transaction{1}
	);

	subscriber.expected_unsubscription_transaction = input_port_activity_subscriber::unsubscription_transaction{1};
	client.handle_response(
		Pipe::worker_sync::port_activity_unsubscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{0},
		ec
	);

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);

	subscriber.expected_conn_lost_ptr = &client;
}
