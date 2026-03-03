//@	{"target":{"name":"transaction.test"}}

#include "./transaction.hpp"

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
