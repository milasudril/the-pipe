//@	{"target":{"name": "fifo.test"}}

#include "./fifo.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_utils_fifo_push_and_drain_all_elements)
{
	Pipe::utils::fifo<int> q;
	q.push(1);
	q.push(2);
	q.push(3);

	std::vector<int> results;
	q.drain_until_blocked_or_empty([&](int val) {
		results.push_back(val);
		return true;
	});

	std::vector<int> expected{1, 2, 3};
	EXPECT_EQ(results, expected);
}

TESTCASE(Pipe_utils_fifo_push_and_drain_until_callback_returns_false) {
	Pipe::utils::fifo<int> q;
	q.push(10);
	q.push(20);
	q.push(30);

	size_t count = 0;
	q.drain_until_blocked_or_empty(
		[&](int) {
			if (count == 1)
			{return false; }

			++count;
			return true;
		}
	);

	EXPECT_EQ(count, 1);

	q.drain_until_blocked_or_empty(
		[](int val)
		{
			EXPECT_EQ(val, 20);
			return false;
		}
	);
}

TESTCASE(Pipe_utils_fifo_shift_when_half_empty)
{
	Pipe::utils::fifo<int> q;
	q.push(1);
	q.push(2);
	q.push(3);
	q.push(4);

	size_t drained = 0;
	int const* first_address = nullptr;
	q.drain_until_blocked_or_empty([&](int const& val) {
		first_address = first_address == nullptr? &val :first_address;
		++drained;
		return drained <= 2;
	});

	q.drain_until_blocked_or_empty([&](int const& val) {
		EXPECT_EQ(&val, first_address);
    EXPECT_EQ(val, 3);
		return false;
	});
}

TESTCASE(Pipe_utils_fifo_handles_move_only_types)
{
	Pipe::utils::fifo<std::unique_ptr<int>> ptr_q;
	ptr_q.push(std::make_unique<int>(100));
	ptr_q.push(std::make_unique<int>(200));
	ptr_q.push(std::make_unique<int>(300));

	ptr_q.drain_until_blocked_or_empty(
		[](std::unique_ptr<int>& ptr)
		{
			REQUIRE_NE(ptr, nullptr);
			EXPECT_EQ(*ptr, 100);
			return false;
		}
	);
}

TESTCASE(Pipe_utils_fifo_works_with_custom_allocator)
{
	Pipe::utils::fifo<int, std::allocator<int>> q;
	q.push(100);
	q.push(200);
	q.push(300);
	q.push(400);

	int const* first_address = nullptr;

	q.drain_until_blocked_or_empty(
		[&, counter = 0](int const& item) mutable
		{
			if(counter == 0)
			{
				EXPECT_EQ(item, 100);
				first_address = &item;
				++counter;
				return true;
			}
			return false;
		}
	);

	q.drain_until_blocked_or_empty(
		[&](int const& item)
		{
			EXPECT_EQ(item, 200);
			REQUIRE_NE(first_address, nullptr);
			EXPECT_NE(first_address, &item);
			return false;
		}
	);
}

TESTCASE(Pipe_utils_fifo_callback_throws_exception)
{
	Pipe::utils::fifo<int, std::allocator<int>> q;
	q.push(100);
	q.push(200);
	q.push(300);

	try
	{
		q.drain_until_blocked_or_empty(
			[counter = 0](int item) mutable
			{
				if(counter == 1)
				{
					EXPECT_EQ(item, 100);
					++counter;
					return true;
				}
				else
				{ throw std::runtime_error{"My exception"}; }
			}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"My exception"});
	}
}