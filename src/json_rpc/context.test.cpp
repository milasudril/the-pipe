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
		size_t remaining_calls{0};
		std::vector<jopp::object> saved_notifications;

		template<class T>
		void handle_json_rpc_notification(jopp::object&& object)
		{
			EXPECT_GT(remaining_calls, 0);
			--remaining_calls;
			saved_notifications.push_back(std::move(object));
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
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

	notification_handler.remaining_calls = 1;
	ctxt.handle_messages(std::move(notification), std::ref(notification_handler));
	EXPECT_EQ(
		notification_handler.saved_notifications.at(0).get_field_as<jopp::string>("method"),
		"my_notification"
	);
	auto const& recv_params = notification_handler.saved_notifications.at(0).get_field_as<jopp::object>("params");
	EXPECT_EQ(recv_params.get_field_as<jopp::number>("a_param"), 0.2);
}

TESTCASE(Pipe_json_rpc_context_context_handle_message_not_expected)
{
	Pipe::json_rpc::context ctxt;
	my_notification_handler notification_handler;
	jopp::object response;
	response.insert("id", 45.0);
	response.insert("result", 42.0);
	response.insert("jsonrpc", "2.0");

	try
	{
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
		abort();
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"No JSON-RPC response expected"});
	}
}

TESTCASE(Pipe_json_rpc_context_context_send_request_and_handle_message_with_unknown_id)
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(last_response, 1);
		EXPECT_EQ(ctxt.num_pending_responses(), 1);
	}

	{
		jopp::object response;
		response.insert("id", 0.0);
		response.insert("result", 43.0);
		response.insert("jsonrpc", "2.0");
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
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
		ctxt.handle_messages(std::move(response), std::ref(notification_handler));
		EXPECT_EQ(last_response, 0);
		EXPECT_EQ(ctxt.num_pending_responses(), 0);
	}
}

TESTCASE(Pipe_json_rpc_context_context_handle_multiple_messages)
{
	Pipe::json_rpc::context ctxt;
	my_notification_handler notification_handler;

	jopp::array notifications;
	for(size_t k = 0; k != 5; ++k)
	{
		jopp::object params;
		params.insert("value", static_cast<jopp::number>(k));
		jopp::object notification;
		notification.insert("params" ,std::move(params));
		notification.insert("method", "Foobar");
		notification.insert("jsonrpc", "2.0");
		notifications.push_back(std::move(notification));
	}

	notification_handler.remaining_calls = 5;
	ctxt.handle_messages(std::move(notifications), std::ref(notification_handler));

	EXPECT_EQ(std::size(notification_handler.saved_notifications), 5);
	for(size_t k = 0; k != std::size(notification_handler.saved_notifications); ++k)
	{
		auto& notification = notification_handler.saved_notifications[k];
		EXPECT_EQ(
			notification.get_field_as<jopp::string>("method"),
			"Foobar"
		);

		auto const& recv_params = notification.get_field_as<jopp::object>("params");
		EXPECT_EQ(recv_params.get_field_as<jopp::number>("value"), static_cast<jopp::number>(k));
	}
}

TESTCASE(Pipe_json_rpc_context_context_handle_multiple_messages_in_jopp_container)
{
	Pipe::json_rpc::context ctxt;
	my_notification_handler notification_handler;

	jopp::array notifications;
	for(size_t k = 0; k != 5; ++k)
	{
		jopp::object params;
		params.insert("value", static_cast<jopp::number>(k));
		jopp::object notification;
		notification.insert("params" ,std::move(params));
		notification.insert("method", "Foobar");
		notification.insert("jsonrpc", "2.0");
		notifications.push_back(std::move(notification));
	}

	notification_handler.remaining_calls = 5;
	ctxt.handle_messages(jopp::container{std::move(notifications)}, std::ref(notification_handler));

	EXPECT_EQ(std::size(notification_handler.saved_notifications), 5);
	for(size_t k = 0; k != std::size(notification_handler.saved_notifications); ++k)
	{
		auto& notification = notification_handler.saved_notifications[k];
		EXPECT_EQ(
			notification.get_field_as<jopp::string>("method"),
			"Foobar"
		);

		auto const& recv_params = notification.get_field_as<jopp::object>("params");
		EXPECT_EQ(recv_params.get_field_as<jopp::number>("value"), static_cast<jopp::number>(k));
	}
}

namespace
{
	struct my_request
	{
		std::string request_string;
	};

	struct my_response
	{
		double response_value;
	};
}

namespace Pipe::json_rpc
{
	template<>
	struct request_traits<my_request>
	{
		static constexpr char const* method = "my_request";

		static jopp::object params_to_jopp_object(my_request&& request)
		{
			jopp::object ret;
			ret.insert("request_string", std::move(request).request_string);
			return ret;
		}

		static my_response make_response(jopp::value&& value)
		{
			return my_response{
				.response_value = value.get<jopp::object>().get_field_as<jopp::number>("response_value")
			};
		}

		static my_request make_request(jopp::object*)
		{return my_request{};}
	};
};

TESTCASE(Pipe_json_rpc_context_send_object_as_request_with_converted_response)
{
	Pipe::json_rpc::context ctxt;
	my_receiver receiver;

	auto callback_called = false;
	ctxt.send_request(
		std::ref(receiver),
		my_request{"The a answer to the question of universe and anything"},
		[&](my_response val){
			callback_called = true;
			EXPECT_EQ(val.response_value, 42.0);
		}
	);

	jopp::object response;
	response.insert("id", 0.0);
	response.insert("jsonrpc", "2.0");
	jopp::object result;
	result.insert("response_value", 42.0);
	response.insert("result", std::move(result));
	EXPECT_EQ(callback_called, false);
	ctxt.handle_messages(std::move(response), my_notification_handler{});
	EXPECT_EQ(callback_called, true);
}