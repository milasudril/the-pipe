//@	{"target":{"name":"wrapped_request.test"}}

#include "./wrapped_request.hpp"

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