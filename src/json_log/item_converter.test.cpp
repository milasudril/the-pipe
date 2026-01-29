//@	{"target": {"name": "item_converter.test"}}

#include "./item_converter.hpp"
#include "src/log/log.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_log_item_converter_to_jopp_object)
{
	auto obj = Pipe::json_log::to_jopp_object(
		Pipe::log::item{
			.when = Pipe::log::clock::time_point{} + std::chrono::seconds{1},
			.severity = Pipe::log::item::severity::info,
			.message = "This is a test"
		}
	);

	EXPECT_EQ(obj.get_field_as<double>("when"), 1.0);
	EXPECT_EQ(obj.get_field_as<std::string>("severity"), "info");
	EXPECT_EQ(obj.get_field_as<std::string>("message"), "This is a test");
}

TESTCASE(Pipe_json_log_item_converter_make_log_item_good)
{
	jopp::object obj;
	obj.insert("when", 1.0);
	obj.insert("message", "This is a test");
	obj.insert("severity", "info");

	auto const item = Pipe::json_log::make_log_item(obj);
	REQUIRE_EQ(item.has_value(), true);
	EXPECT_EQ(item->when, Pipe::log::clock::time_point{} + std::chrono::seconds{1})
	EXPECT_EQ(item->message, "This is a test");
	EXPECT_EQ(item->severity, Pipe::log::item::severity::info);
}


TESTCASE(Pipe_json_log_item_converter_make_log_item_missing_when)
{
	jopp::object obj;
	obj.insert("message", "This is a test");
	obj.insert("severity", "info");

	auto const item = Pipe::json_log::make_log_item(obj);
	REQUIRE_EQ(item.has_value(), false);
	EXPECT_EQ(
		std::string_view{item.error()},
		"Failed to extract mandatory field `when` from received log item"
	);
}

TESTCASE(Pipe_json_log_item_converter_make_log_item_missing_severity)
{
	jopp::object obj;
	obj.insert("when", 1.0);
	obj.insert("message", "This is a test");

	auto const item = Pipe::json_log::make_log_item(obj);
	REQUIRE_EQ(item.has_value(), false);
	EXPECT_EQ(
		std::string_view{item.error()},
		"Failed to extract mandatory field `severity` from received log item"
	);
}

TESTCASE(Pipe_json_log_item_converter_make_log_item_unknown_severity)
{
	jopp::object obj;
	obj.insert("when", 1.0);
	obj.insert("message", "This is a test");
	obj.insert("severity", "bajs");

	auto const item = Pipe::json_log::make_log_item(obj);
	REQUIRE_EQ(item.has_value(), true);
	EXPECT_EQ(item->severity, Pipe::log::item::severity::info);
}

TESTCASE(Pipe_json_log_item_converter_make_log_item_missing_message)
{
	jopp::object obj;
	obj.insert("when", 1.0);
	obj.insert("severity", "bajs");

	auto const item = Pipe::json_log::make_log_item(obj);
	REQUIRE_EQ(item.has_value(), false);
	EXPECT_EQ(
		std::string_view{item.error()},
		"Failed to extract mandatory field `severity` from received log item"
	);
}