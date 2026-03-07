//@	{"target":{"name":"./json_rpc.test"}}

#include "./json_rpc.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_rpc_make_notification_from_method_and_object)
{
	jopp::object params;
	params.insert("value", 0.5);
	auto notification = Pipe::json_rpc::make_notification("foo", std::move(params));
	EXPECT_EQ(notification.get_field_as<jopp::string>("jsonrpc"), "2.0");
	EXPECT_EQ(notification.get_field_as<jopp::string>("method"), "foo");
	EXPECT_EQ(notification.contains("id"), false);
	auto& params_in_obj = notification.get_field_as<jopp::object>("params");
	EXPECT_EQ(params_in_obj.get_field_as<jopp::number>("value"), 0.5);
}

namespace
{
	struct status_notification
	{
		double value;
	};
}

template<>
struct Pipe::json_rpc::notification_traits<status_notification>
{
	static constexpr char const* method = "status_notification";
	static jopp::object params(status_notification obj)
	{
		jopp::object ret;
		ret.insert("value", obj.value);
		return ret;
	}
};
TESTCASE(Pipe_json_rpc_make_notification_with_deduced_method_name)
{
	auto notification = Pipe::json_rpc::make_notification(
		status_notification{
			.value = 0.25
		}
	);
	EXPECT_EQ(notification.get_field_as<jopp::string>("jsonrpc"), "2.0");
	EXPECT_EQ(notification.get_field_as<jopp::string>("method"), "status_notification");
	EXPECT_EQ(notification.contains("id"), false);
	auto& params_in_obj = notification.get_field_as<jopp::object>("params");
	EXPECT_EQ(params_in_obj.get_field_as<jopp::number>("value"), 0.25);
}

TESTCASE(Pipe_json_rpc_make_response_from_req_with_id)
{
	jopp::object my_request{};
	my_request.insert("method", "foo");
	my_request.insert("id", 35.0);
	auto response = make_response(Pipe::json_rpc::request{std::move(my_request)});
	EXPECT_EQ(response.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(response.get_field_as<double>("id"), 35.0);
}

TESTCASE(Pipe_json_rpc_make_response_with_result)
{
	jopp::object my_request{};
	my_request.insert("method", "foo");
	my_request.insert("id", 35.0);
	jopp::object result;
	result.insert("value", 42.0);
	auto response = make_response(Pipe::json_rpc::request{std::move(my_request)}, std::move(result));
	EXPECT_EQ(response.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(response.get_field_as<double>("id"), 35.0);
	auto const& stored_result = response.get_field_as<jopp::object>("result");
	EXPECT_EQ(stored_result.get_field_as<double>("value"), 42.0);
}

TESTCASE(Pipe_json_rpc_make_response_with_error)
{
	jopp::object my_request{};
	my_request.insert("method", "foo");
	my_request.insert("id", 35.0);

	auto response = make_response(
		Pipe::json_rpc::request{std::move(my_request)},
		std::runtime_error{"Something went wrong"},
		143
	);

	EXPECT_EQ(response.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(response.get_field_as<double>("id"), 35.0);
	auto const& stored_result = response.get_field_as<jopp::object>("error");
	EXPECT_EQ(stored_result.get_field_as<double>("code"), 143.0);
	EXPECT_EQ(stored_result.get_field_as<std::string>("message"), "Something went wrong");
}
