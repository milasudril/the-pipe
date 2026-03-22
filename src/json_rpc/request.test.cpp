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

#if 0
	template<class Func, class ErrorHandler>
	void handle_message(jopp::object&& obj, Func&& func, ErrorHandler&& on_error)
	{
		auto const i = obj.find("id");
		if(i != std::end(obj))
		{
			auto id = std::move(i->second);
			auto params = obj.try_get_field_as<jopp::object>("params");
			try
			{
				std::forward<Func>(func)(
					received_request{
						.method = std::move(obj.get_field_as<jopp::string>("method")),
						.params = (params == nullptr)? jopp::object{} : std::move(*params),
						.id = copy_id(id)
					}
				);
			}
			catch(std::exception const& err)
			{
				std::forward<ErrorHandler>(on_error)(
					message_handling_error{
						.what = err.what(),
						.id = std::move(id)
					}
				);
				return;
			}
		}
		else
		{
			try
			{
				auto params = obj.try_get_field_as<jopp::object>("params");
				std::forward<Func>(func)(
					received_notification{
						.method = std::move(obj.get_field_as<jopp::string>("method")),
						.params = (params == nullptr)? jopp::object{} : std::move(*params)
					}
				);
			}
			catch(std::exception const& err)
			{
				std::forward<ErrorHandler>(on_error)(
					message_handling_error{
						.what = err.what(),
						.id = jopp::value{}
					}
				);
				return;
			}
		}
	}
#endif

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
