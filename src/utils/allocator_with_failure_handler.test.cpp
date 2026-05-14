//@	{"target":{"name":"allocator_with_failure_handler.test"}}

#include "./allocator_with_failure_handler.hpp"

#include <testfwk/testfwk.hpp>
#include <dlfcn.h>
#include <deque>
#include <memory>

namespace
{
	struct allocation_failure_handler
	{
		void raise_byte_size_computation_error(std::type_identity<int>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to compute the size of {} ints", n)};
		}

		void raise_memory_allocation_error(std::type_identity<int>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to allocate {} ints", n)};
		}
	};

	struct allocation_failure_handler_2
	{
		allocation_failure_handler_2(int){}

		void raise_byte_size_computation_error(std::type_identity<int>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to compute the size of {} ints", n)};
		}

		void raise_byte_size_computation_error(std::type_identity<int*>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to compute the size of {} ints", n)};
		}

		void raise_memory_allocation_error(std::type_identity<int>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to allocate {} ints", n)};
		}

		void raise_memory_allocation_error(std::type_identity<int*>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to allocate {} int*s", n)};
		}
	};
	bool fail_next_malloc = false;
}

extern "C"
{
	void* malloc(size_t n)
	{
		if(fail_next_malloc)
		{
			fail_next_malloc = false;
			return nullptr;
		}

		auto real_malloc = reinterpret_cast<void* (*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
		return real_malloc(n);
	}
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_out_of_vm)
{
	try
	{
		std::vector<
			int,
			Pipe::utils::allocator_with_failure_handler<int, allocation_failure_handler>
		>
		buffer(4611686018427387905ll);
		REQUIRE_EQ(false, true);
	}
	catch(std::length_error const&)
	{}
	catch(std::bad_array_new_length const&)
	{}
	catch(...)
	{ REQUIRE_EQ(false, true); }
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_malloc_succeeds)
{
	std::vector<
		int,
		Pipe::utils::allocator_with_failure_handler<int, allocation_failure_handler>
	>
	buffer;
	buffer.push_back(235);
	EXPECT_EQ(std::size(buffer), 1);
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_malloc_fails)
{
	std::vector<
		int,
		Pipe::utils::allocator_with_failure_handler<int, allocation_failure_handler>
	>
	buffer;
	fail_next_malloc = true;
	try
	{
		buffer.push_back(235);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to allocate 1 ints"});
	}
	EXPECT_EQ(std::size(buffer), 0);
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_non_default_constructible_handler)
{
	using allocator =
		Pipe::utils::allocator_with_failure_handler<
			int,
			allocation_failure_handler_2
		>;

	std::vector<int, allocator> buffer(allocator{allocation_failure_handler_2{123}});
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_non_default_constructible_handler_deque)
{
	using allocator =
		Pipe::utils::allocator_with_failure_handler<
			int,
			allocation_failure_handler_2
		>;

	std::deque<
		int,
		allocator
	>
	buffer(allocator{allocation_failure_handler_2{123}});
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_pointer_set_to_null)
{
	using allocator =
		Pipe::utils::allocator_with_failure_handler<
			int,
			std::shared_ptr<allocation_failure_handler_2>
		>;

	std::vector<int, allocator> buffer;

	fail_next_malloc = true;
	try
	{
		buffer.push_back(235);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"std::bad_alloc"});
	}
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_pointer_not_set_to_null)
{
	using allocator =
		Pipe::utils::allocator_with_failure_handler<
			int,
			std::shared_ptr<allocation_failure_handler_2>
		>;

	std::vector<int, allocator> buffer{allocator{std::make_shared<allocation_failure_handler_2>(345)}};

	fail_next_malloc = true;
	try
	{
		buffer.push_back(235);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to allocate 1 ints"});
	}
}

TESTCASE(Pipe_utils_allocator_with_failure_handler_manual_call_to_allocate)
{
	Pipe::utils::allocator_with_failure_handler<int, allocation_failure_handler> allocator{};
	try
	{
		std::ignore = allocator.allocate(4611686018427387905ll);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to compute the size of 4611686018427387905 ints"});
	}
}