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
	static jopp::object params_to_jopp_object(status_notification obj)
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
