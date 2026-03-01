//@	{"target":{"name":"./json_rpc.test"}}

#include "./json_rpc.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_rpc_transaction_id_default_value)
{
	EXPECT_EQ(Pipe::json_rpc::transaction_id{}.value(), 0.0);
}

TESTCASE(Pipe_json_rpc_transaction_id_from_value)
{
	Pipe::json_rpc::transaction_id id{234};
	EXPECT_EQ(id.value(), 234.0);

	try
	{
		Pipe::json_rpc::transaction_id invalid{-1};
		abort();
	}
	catch(...)
	{}

	Pipe::json_rpc::transaction_id largest{jopp::max_safe_integer};
	EXPECT_EQ(largest.value(), static_cast<double>(jopp::max_safe_integer));

	try
	{
		Pipe::json_rpc::transaction_id invalid{jopp::max_safe_integer + 1};
		abort();
	}
	catch(...)
	{}
}

TESTCASE(Pipe_json_rpc_transaction_id_next)
{
	Pipe::json_rpc::transaction_id id{234};
	EXPECT_EQ(id.value(), 234.0);
	auto current = id.next();
	EXPECT_GT(id, current);
}

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

TESTCASE(Pipe_json_rpc_request_from_jopp_object)
{
	jopp::object obj;
	obj.insert("method", "foobar");
	Pipe::json_rpc::request req{std::move(obj)};
	EXPECT_EQ(req.method(), "foobar");

	auto const stored_obj = req.take_value();
	EXPECT_EQ(stored_obj.get_field_as<std::string>("method"), "foobar");
}

TESTCASE(Pipe_json_rpc_request_from_jopp_object_required_fields_missing)
{
	try
	{
		jopp::object obj;
		Pipe::json_rpc::request req{std::move(obj)};
		abort();
	}
	catch(...)
	{}
}

TESTCASE(Pipe_json_rpc_request_from_id_method_params)
{
	jopp::object params{};
	params.insert("my_param", 64.0);
	Pipe::json_rpc::request req{Pipe::json_rpc::transaction_id{34}, "foobar", std::move(params)};
	EXPECT_EQ(req.method(), "foobar");

	auto stored_obj = req.take_value();
	EXPECT_EQ(stored_obj.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(stored_obj.get_field_as<double>("id"), 34.0);
	EXPECT_EQ(stored_obj.get_field_as<std::string>("method"), "foobar");

	auto const& stored_params = stored_obj.get_field_as<jopp::object>("params");
	EXPECT_EQ(stored_params.get_field_as<double>("my_param"), 64.0);
}

TESTCASE(Pipe_json_rpc_context_make_request)
{
	Pipe::json_rpc::context ctxt;

	jopp::object params_a;
	params_a.insert("my_param", 64.0);
	auto req_a = ctxt.make_request("method_a", std::move(params_a));
	EXPECT_EQ(req_a.first, Pipe::json_rpc::transaction_id{});
	auto const req_a_obj = req_a.second.take_value();
	EXPECT_EQ(req_a_obj.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(req_a_obj.get_field_as<double>("id"), 0.0);
	EXPECT_EQ(req_a_obj.get_field_as<std::string>("method"), "method_a");
	auto const& req_a_params = req_a_obj.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_a_params.get_field_as<double>("my_param"), 64.0);

	jopp::object params_b;
	params_b.insert("my_other_param", 32.0);
	auto req_b = ctxt.make_request("method_b", std::move(params_b));
	EXPECT_EQ(req_b.first, Pipe::json_rpc::transaction_id{1});
	auto const req_b_obj = req_b.second.take_value();
	EXPECT_EQ(req_b_obj.get_field_as<std::string>("jsonrpc"), "2.0");
	EXPECT_EQ(req_b_obj.get_field_as<double>("id"), 1.0);
	EXPECT_EQ(req_b_obj.get_field_as<std::string>("method"), "method_b");
	auto const& req_b_params = req_b_obj.get_field_as<jopp::object>("params");
	EXPECT_EQ(req_b_params.get_field_as<double>("my_other_param"), 32.0);
}

TESTCASE(Pipe_json_rpc_make_response_from_req_without_id)
{
	jopp::object my_request{};
	my_request.insert("method", "foo");
	auto response = make_response(Pipe::json_rpc::request{std::move(my_request)});
	EXPECT_EQ(response.get_field_as<std::string>("jsonrpc"), "2.0");
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
