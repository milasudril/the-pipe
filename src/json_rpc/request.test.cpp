//@	{"target":{"name":"request.test"}}

#include "./request.hpp"
#include "src/utils/utils.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_rpc_ensure_required_fields_obj_without_method)
{
	try
	{
		auto val = Pipe::json_rpc::ensure_required_fields(jopp::object{});
		abort();
	}
	catch(...)
	{}
}

TESTCASE(Pipe_json_rpc_ensure_required_fields_obj_required_fields_present)
{
	jopp::object obj_to_test;
	obj_to_test.insert("method", "foobar");
	auto val = Pipe::json_rpc::ensure_required_fields(std::move(obj_to_test));
	EXPECT_EQ(val.contains("method"), true);
}

TESTCASE(Pipe_json_rpc_wrapped_request_from_jopp_object)
{
	jopp::object obj;
	obj.insert("method", "foobar");
	Pipe::json_rpc::wrapped_request req{std::move(obj)};
	EXPECT_EQ(req.method(), "foobar");

	auto const stored_obj = req.take_value();
	EXPECT_EQ(stored_obj.get_field_as<std::string>("method"), "foobar");
}

TESTCASE(Pipe_json_rpc_wrapped_request_from_jopp_object_required_fields_missing)
{
	try
	{
		jopp::object obj;
		Pipe::json_rpc::wrapped_request req{std::move(obj)};
		abort();
	}
	catch(...)
	{}
}

TESTCASE(Pipe_json_rpc_wrapped_request_from_id_method_params)
{
	jopp::object params{};
	params.insert("my_param", 64.0);
	Pipe::json_rpc::wrapped_request req{Pipe::json_rpc::transaction_id{34}, "foobar", std::move(params)};
	EXPECT_EQ(req.method(), "foobar");

	auto stored_obj = req.take_value();
	EXPECT_EQ(stored_obj.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(stored_obj.get_field_as<double>("id"), 34.0);
	EXPECT_EQ(stored_obj.get_field_as<std::string>("method"), "foobar");

	auto const& stored_params = stored_obj.get_field_as<jopp::object>("params");
	EXPECT_EQ(stored_params.get_field_as<double>("my_param"), 64.0);
}

TESTCASE(Pipe_json_rpc_handle_message_request_no_params_no_method)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj{};
	obj.insert("id", 12.0);

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(err.id, jopp::value{12.0});
	EXPECT_EQ(err.message, std::string_view{"Mandatory field `method` is missing"});
}

TESTCASE(Pipe_json_rpc_handle_message_request_no_params)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("id", 12.0);
	obj.insert("method", "Foobar");

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 1);
	auto& request = std::get<Pipe::json_rpc::received_request>(msg);
	EXPECT_EQ(request.method, std::string_view{"Foobar"});
	EXPECT_EQ(request.params.size(), 0);
	EXPECT_EQ(request.id.get<jopp::number>(), 12.0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message.empty(), true);
}


TESTCASE(Pipe_json_rpc_handle_message_request_params_wrong_type)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	obj.insert("params", "Hej");
	obj.insert("id", 12.0);

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(err.id, jopp::value{12.0});
	EXPECT_EQ(err.message, std::string_view{"The field `params` has wrong type"});
}

TESTCASE(Pipe_json_rpc_handle_message_request)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	jopp::object params;
	params.insert("value", 124.0);
	obj.insert("params", std::move(params));
	obj.insert("id", 12.0);

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 1);
	auto& request = std::get<Pipe::json_rpc::received_request>(msg);
	EXPECT_EQ(request.method, std::string_view{"Foobar"});
	EXPECT_EQ(request.params.get_field_as<jopp::number>("value"), 124.0);
	EXPECT_EQ(request.id.get<jopp::number>(), 12.0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message.empty(), true);
}

TESTCASE(Pipe_json_rpc_handle_message_request_exception_thrown)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	jopp::object params;
	params.insert("value", 124.0);
	obj.insert("params", std::move(params));
	obj.insert("id", 12.0);

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
			throw std::runtime_error{"Something went wrong"};
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 1);
	auto& request = std::get<Pipe::json_rpc::received_request>(msg);
	EXPECT_EQ(request.method, std::string_view{"Foobar"});
	EXPECT_EQ(request.params.get_field_as<jopp::number>("value"), 124.0);
	EXPECT_EQ(err.id, jopp::value{12.0});
	EXPECT_EQ(err.message, std::string_view{"Something went wrong"});
}

TESTCASE(Pipe_json_rpc_handle_message_notification_no_params_no_method)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	Pipe::json_rpc::handle_message(
		jopp::object{},
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message, std::string_view{"Mandatory field `method` is missing"});
}

TESTCASE(Pipe_json_rpc_handle_message_notification_no_params)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 2);
	auto& notification = std::get<Pipe::json_rpc::received_notification>(msg);
	EXPECT_EQ(notification.method, std::string_view{"Foobar"});
	EXPECT_EQ(notification.params.size(), 0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message.empty(), true);
}


TESTCASE(Pipe_json_rpc_handle_message_notification_params_wrong_type)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	obj.insert("params", "Hej");

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(msg.index(), 0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message, std::string_view{"The field `params` has wrong type"});
}

TESTCASE(Pipe_json_rpc_handle_message_notification)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	jopp::object params;
	params.insert("value", 124.0);
	obj.insert("params", std::move(params));

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 2);
	auto& notification = std::get<Pipe::json_rpc::received_notification>(msg);
	EXPECT_EQ(notification.method, std::string_view{"Foobar"});
	EXPECT_EQ(notification.params.get_field_as<jopp::number>("value"), 124.0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message.empty(), true);
}

TESTCASE(Pipe_json_rpc_handle_message_notification_exception_thrown)
{
	std::variant<
		std::monostate,
		Pipe::json_rpc::received_request,
		Pipe::json_rpc::received_notification
	> msg;

	Pipe::json_rpc::message_handling_error err;

	jopp::object obj;
	obj.insert("method", "Foobar");
	jopp::object params;
	params.insert("value", 124.0);
	obj.insert("params", std::move(params));

	Pipe::json_rpc::handle_message(
		std::move(obj),
		[&msg]<class T>(T&& value){
			msg = std::move(value);
			throw std::runtime_error{"Something went wrong"};
		},
		[&err](Pipe::json_rpc::message_handling_error&& value){
			err = std::move(value);
		}
	);

	EXPECT_EQ(msg.index(), 2);
	auto& notification = std::get<Pipe::json_rpc::received_notification>(msg);
	EXPECT_EQ(notification.method, std::string_view{"Foobar"});
	EXPECT_EQ(notification.params.get_field_as<jopp::number>("value"), 124.0);
	EXPECT_EQ(err.id, jopp::value{});
	EXPECT_EQ(err.message, std::string_view{"Something went wrong"});
}
