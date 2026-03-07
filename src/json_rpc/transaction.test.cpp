//@	{"target":{"name":"transaction.test"}}

#include "./transaction.hpp"
#include "testfwk/testsuite.hpp"

#include <jopp/types.hpp>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_rpc_transaction_id_default_value)
{
	EXPECT_EQ(Pipe::json_rpc::transaction_id{}.value(), 0.0);
}

TESTCASE(Pipe_json_rpc_transaction_id_from_float)
{
	Pipe::json_rpc::transaction_id id{234.0};
	EXPECT_EQ(id.value(), 234.0);

	try
	{
		Pipe::json_rpc::transaction_id invalid{-1.0};
		abort();
	}
	catch(...)
	{}

	Pipe::json_rpc::transaction_id largest{jopp::max_safe_integer};
	EXPECT_EQ(largest.value(), static_cast<double>(jopp::max_safe_integer));

	try
	{
		Pipe::json_rpc::transaction_id invalid{static_cast<double>(jopp::max_safe_integer) + 4.0};
		abort();
	}
	catch(...)
	{}

	try
	{
		Pipe::json_rpc::transaction_id invalid{4.123};
		abort();
	}
	catch(...)
	{}
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

	try
	{
		auto const val = std::bit_cast<double>(0x7ff0000000000001);
		EXPECT_EQ(std::isnan(val), true);
		Pipe::json_rpc::transaction_id invalid{std::bit_cast<double>(val)};
		abort();
	}
	catch(...)
	{}

	try
	{
		auto const val = 1.0/0.0;
		EXPECT_EQ(std::isinf(val), true);
		Pipe::json_rpc::transaction_id invalid{std::bit_cast<double>(val)};
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

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_no_jsonrpc_field)
{
	Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
	try
	{
		transaction.finalize(jopp::object{});
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Mandatory field `jsonrpc` is missing"});
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_no_jsonrpc_wrong_type)
{
	Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
	try
	{
		jopp::object response;
		response.insert("jsonrpc", 2.0);
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Field `jsonrpc` should be a string"});
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_no_jsonrpc_wrong_value)
{
	Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
	try
	{
		jopp::object response;
		response.insert("jsonrpc", "2.3");
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Unsupported JSON-RPC version"});
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_no_payload)
{
	Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
	try
	{
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"A JSON-RPC response must contain either a `result` or an `error` object"});
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_both_result_and_error)
{
	Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
	try
	{
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		response.insert("result", jopp::object{});
		response.insert("error", jopp::object{});
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"A JSON-RPC response must contain either a `result` or an `error` object"});
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_result)
{
	bool called = false;
	Pipe::json_rpc::transaction transaction{[&called](jopp::value const& result){
		auto const& obj = result.get<jopp::object>();
		EXPECT_EQ(obj.get_field_as<jopp::string>("foo"), "bar");
		called = true;
	}};

	jopp::object response;
	response.insert("jsonrpc", "2.0");
	jopp::object result{};
	result.insert("foo", "bar");
	response.insert("result", jopp::object{std::move(result)});
	transaction.finalize(std::move(response));
	EXPECT_EQ(called, true);
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_result_nullptr)
{
	bool called = false;
	Pipe::json_rpc::transaction transaction{[&called](jopp::value const& result){
		EXPECT_NE(result.get_if<jopp::null>(), nullptr);
		called = true;
	}};

	jopp::object response;
	response.insert("jsonrpc", "2.0");
	response.insert("result", jopp::null{});
	transaction.finalize(std::move(response));
	EXPECT_EQ(called, true);
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_error_missing_fields)
{
	try
	{
		Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		response.insert("error", jopp::object{});
		transaction.finalize(std::move(response));
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{
				"A JSON-RPC error must consist of a numeric `code` and a string `message`"
			}
		);
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_error_missing_message)
{
	try
	{
		Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		jopp::object error;
		error.insert("code", 124.0);
		response.insert("error", std::move(error));
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{
				"A JSON-RPC error must consist of a numeric `code` and a string `message`"
			}
		);
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_error_missing_code)
{
	try
	{
		Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		jopp::object error;
		error.insert("message", "Foobar");
		response.insert("error", std::move(error));
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{
				"A JSON-RPC error must consist of a numeric `code` and a string `message`"
			}
		);
	}
}

TESTCASE(Pipe_json_rpc_transaction_finalize_response_with_error)
{
	try
	{
		Pipe::json_rpc::transaction transaction{[](jopp::value&&){}};
		jopp::object response;
		response.insert("jsonrpc", "2.0");
		jopp::object error;
		error.insert("message", "Foobar");
		error.insert("code", 123.0);
		response.insert("error", std::move(error));
		transaction.finalize(std::move(response));
		abort();
	}
	catch(std::runtime_error const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{
				"JSON-RPC remote error 123: Foobar"
			}
		);
	}
}