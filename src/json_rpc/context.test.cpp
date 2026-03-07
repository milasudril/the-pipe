//@	{"target":{"name":"context.test"}}

#include "./context.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct my_receiver
	{
		jopp::object written_object;
		bool throw_at_write = false;
		void write(jopp::object&& obj)
		{
			written_object = std::move(obj);
			if(throw_at_write)
			{ throw std::runtime_error{"Write failed"}; }
		}
	};

	struct my_notification_handler
	{
		bool should_be_called{false};
		jopp::object saved_notification;

		void handle_json_rpc_notification(jopp::object&& object)
		{
			EXPECT_EQ(should_be_called, true);
			should_be_called = false;
			saved_notification = std::move(object);
		}
	};
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_good_response_first_in_queue)
{
	Pipe::json_rpc::context ctxt;
	jopp::object params;
	params.insert("a_param", 0.2);
	my_receiver receiver{};
	auto response_handled = false;
	ctxt.send_request(
		std::ref(receiver),
		"my_request",
		std::move(params),
		[&](jopp::value const& val){
			response_handled = true;
			EXPECT_EQ(val.get<jopp::number>(), 42.0);
		}
	);

	EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
	EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
	auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);

	EXPECT_EQ(ctxt.num_pending_responses(), 1);

	my_notification_handler notification_handler;
	{
		jopp::object response;
		response.insert("id", 0.0);
		response.insert("result", 42.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(response_handled, true);
	}

	EXPECT_EQ(ctxt.num_pending_responses(), 0);
}

TESTCASE(Pipe_json_rpc_context_context_send_request_write_throws)
{
	Pipe::json_rpc::context ctxt;
	jopp::object params;
	params.insert("a_param", 0.2);
	my_receiver receiver{};
	receiver.throw_at_write = true;
	auto response_handled = false;
	try
	{
		ctxt.send_request(
			std::ref(receiver),
			"my_request",
			std::move(params),
			[&](jopp::value const& val){
				response_handled = true;
				EXPECT_EQ(val.get<jopp::number>(), 42.0);
			}
		);
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Write failed"});
	}

	EXPECT_EQ(ctxt.num_pending_responses(), 0);
	EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
	EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
	auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);
	EXPECT_EQ(ctxt.num_pending_responses(), 0);

	EXPECT_EQ(response_handled, false);
	EXPECT_EQ(ctxt.num_pending_responses(), 0);
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_good_response_throws_exception)
{
	Pipe::json_rpc::context ctxt;
	jopp::object params;
	params.insert("a_param", 0.2);
	my_receiver receiver{};
	auto response_handled = false;
	ctxt.send_request(
		std::ref(receiver),
		"my_request",
		std::move(params),
		[&](jopp::value const&){
			response_handled = true;
			throw std::runtime_error{"Failed to handle response"};
		}
	);

	EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
	EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
	auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);

	EXPECT_EQ(ctxt.num_pending_responses(), 1);

	my_notification_handler notification_handler;
	try
	{
		jopp::object response;
		response.insert("id", 0.0);
		response.insert("result", 42.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		abort();
	}
	catch(std::runtime_error const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Failed to handle response"}); }

	EXPECT_EQ(response_handled, true);
	EXPECT_EQ(ctxt.num_pending_responses(), 0);
}

TESTCASE(Pipe_json_rpc_context_context_handle_notification)
{
	Pipe::json_rpc::context ctxt;
	my_notification_handler notification_handler;
	jopp::object params;
	params.insert("a_param", 0.2);
	jopp::object notification;
	notification.insert("method", "my_notification");
	notification.insert("params", std::move(params));

	notification_handler.should_be_called = true;
	ctxt.handle_response(std::move(notification), std::ref(notification_handler));
	EXPECT_EQ(
		notification_handler.saved_notification.get_field_as<jopp::string>("method"),
		"my_notification"
	);
	auto const& recv_params = notification_handler.saved_notification.get_field_as<jopp::object>("params");
	EXPECT_EQ(recv_params.get_field_as<jopp::number>("a_param"), 0.2);
}

TESTCASE(Pipe_json_rpc_context_context_handle_response_not_expected)
{
	Pipe::json_rpc::context ctxt;
	my_notification_handler notification_handler;
	jopp::object response;
	response.insert("id", 45.0);
	response.insert("result", 42.0);
	response.insert("jsonrpc", "2.0");

	try
	{
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"No JSON-RPC response expected"});
	}
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_response_with_unknown_id)
{
	Pipe::json_rpc::context ctxt;
	jopp::object params;
	params.insert("a_param", 0.2);
	my_receiver receiver{};
	auto response_handled = false;
	ctxt.send_request(
		std::ref(receiver),
		"my_request",
		std::move(params),
		[&](jopp::value const& val){
			response_handled = true;
			EXPECT_EQ(val.get<jopp::number>(), 42.0);
		}
	);

	EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
	EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
	auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);

	EXPECT_EQ(ctxt.num_pending_responses(), 1);

	my_notification_handler notification_handler;
	try
	{
		jopp::object response;
		response.insert("id", 45.0);
		response.insert("result", 42.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"JSON-RPC response has an unexpected transaction id"});
	}
	EXPECT_EQ(response_handled, false);

	EXPECT_EQ(ctxt.num_pending_responses(), 1);
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_good_responses_ooo)
{
	Pipe::json_rpc::context ctxt;

	my_receiver receiver{};
	std::optional<int64_t> last_response;
	{
		jopp::object params;
		params.insert("a_param", 0.2);
		ctxt.send_request(
			std::ref(receiver),
			"my_request",
			std::move(params),
			[&](jopp::value const& val){
				last_response = 0;
				EXPECT_EQ(val.get<jopp::number>(), 43.0);
			}
		);

		EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
		EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
		auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
		EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);
	}

	{
		jopp::object params;
		params.insert("a_param", 0.2);
		ctxt.send_request(
			std::ref(receiver),
			"my_request",
			std::move(params),
			[&](jopp::value const& val){
				last_response = 1;
				EXPECT_EQ(val.get<jopp::number>(), 42.0);
			}
		);

		EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 1.0);
		EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
		auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
		EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);
	}

	EXPECT_EQ(ctxt.num_pending_responses(), 2);

	my_notification_handler notification_handler;

	{
		jopp::object response;
		response.insert("id", 1.0);
		response.insert("result", 42.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(last_response, 1);
		EXPECT_EQ(ctxt.num_pending_responses(), 1);
	}

	{
		jopp::object response;
		response.insert("id", 0.0);
		response.insert("result", 43.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(last_response, 0);
		EXPECT_EQ(ctxt.num_pending_responses(), 0);
	}
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_good_responses_ooo_first_response_throws)
{
	Pipe::json_rpc::context ctxt;

	my_receiver receiver{};
	std::optional<int64_t> last_response;
	{
		jopp::object params;
		params.insert("a_param", 0.2);
		ctxt.send_request(
			std::ref(receiver),
			"my_request",
			std::move(params),
			[&](jopp::value const& val){
				last_response = 0;
				EXPECT_EQ(val.get<jopp::number>(), 43.0);
			}
		);

		EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 0.0);
		EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
		auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
		EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);
	}

	{
		jopp::object params;
		params.insert("a_param", 0.2);
		ctxt.send_request(
			std::ref(receiver),
			"my_request",
			std::move(params),
			[&](jopp::value const& val){
				last_response = 1;
				EXPECT_EQ(val.get<jopp::number>(), 42.0);
				throw std::runtime_error{"Failed to process response"};
			}
		);

		EXPECT_EQ(receiver.written_object.get_field_as<jopp::number>("id"), 1.0);
		EXPECT_EQ(receiver.written_object.get_field_as<jopp::string>("method"), "my_request");
		auto const& req_params = receiver.written_object.get_field_as<jopp::object>("params");
		EXPECT_EQ(req_params.get_field_as<jopp::number>("a_param"), 0.2);
	}

	EXPECT_EQ(ctxt.num_pending_responses(), 2);

	my_notification_handler notification_handler;

	try
	{
		jopp::object response;
		response.insert("id", 1.0);
		response.insert("result", 42.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to process response"});
	}
	EXPECT_EQ(last_response, 1);
	EXPECT_EQ(ctxt.num_pending_responses(), 1);

	{
		jopp::object response;
		response.insert("id", 0.0);
		response.insert("result", 43.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_response(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(last_response, 0);
		EXPECT_EQ(ctxt.num_pending_responses(), 0);
	}
}