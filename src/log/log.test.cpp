//@	{"target": {"name": "log.test"}}

#include "./log.hpp"

#include <testfwk/testfwk.hpp>
#include <unistd.h>

TESTCASE(Pipe_log_write_message_null_cfg_is_noop)
{
	write_message(Pipe::log::severity::warning, "Foo {}", 1243);
}

namespace
{
	struct my_writer
	{
		std::vector<Pipe::log::item> written_items;

		void write(Pipe::log::item&& item_to_write)
		{
			written_items.push_back(std::move(item_to_write));
		}
	};

	struct my_timestamp_generator
	{
		Pipe::log::clock::time_point now()
		{
			auto ret = Pipe::log::clock::time_point{} + delta;
			delta += std::chrono::seconds{1};

			return ret;
		}

		Pipe::log::clock::duration delta{};
	};
}

TESTCASE(Pipe_log_configure_and_write_message)
{
	my_timestamp_generator generator;
	my_writer writer;

	{
		Pipe::log::context ctxt{
			Pipe::log::configuration{
				.writer = std::ref(writer),
				.timestamp_generator = std::ref(generator)
			}
		};

		write_message(Pipe::log::severity::info, "This is an info message {}", 1);
		write_message(Pipe::log::severity::warning, "This is a warning message {}", 2);
		write_message(Pipe::log::severity::error, "This is an error message {}", 3);

		EXPECT_EQ(writer.written_items.size(), 3);

		{
			REQUIRE_GE(writer.written_items.size(), 1);
			auto& item = writer.written_items[0];
			EXPECT_EQ(item.when,  Pipe::log::clock::time_point{});
			EXPECT_EQ(item.message, "This is an info message 1");
			EXPECT_EQ(item.severity, Pipe::log::severity::info);
		}

		{
			REQUIRE_GE(writer.written_items.size(), 2);
			auto& item = writer.written_items[1];
			EXPECT_EQ(item.when,  Pipe::log::clock::time_point{} + std::chrono::seconds{1});
			EXPECT_EQ(item.message, "This is a warning message 2");
			EXPECT_EQ(item.severity, Pipe::log::severity::warning);
		}

		{
			REQUIRE_GE(writer.written_items.size(), 3);
			auto& item = writer.written_items[2];
			EXPECT_EQ(item.when,  Pipe::log::clock::time_point{} + std::chrono::seconds{2});
			EXPECT_EQ(item.message, "This is an error message 3");
			EXPECT_EQ(item.severity, Pipe::log::severity::error);
		}
	}

	auto written_items = writer.written_items;
	write_message(Pipe::log::severity::info, "This is an info message {}", 1);
	EXPECT_EQ(writer.written_items, written_items);
}