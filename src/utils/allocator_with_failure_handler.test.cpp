//@	{"target":{"name":"allocator_with_failure_handler.test"}}

#include "./allocator_with_failure_handler.hpp"

#include <testfwk/testfwk.hpp>
#include <dlfcn.h>

namespace
{
	struct allocation_failure_handler
	{
		void memory_allocation_failed(std::type_identity<int>, size_t n)
		{
			throw std::runtime_error{std::format("Failed to allocate {} ints", n)};
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

TESTCASE(Pipe_uitls_allocator_with_failure_handler_out_of_vm)
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

TESTCASE(Pipe_uitls_allocator_with_failure_handler_malloc_succeeds)
{
	std::vector<
		int,
		Pipe::utils::allocator_with_failure_handler<int, allocation_failure_handler>
	>
	buffer;
	buffer.push_back(235);
	EXPECT_EQ(std::size(buffer), 1);
}
TESTCASE(Pipe_uitls_allocator_with_failure_handler_malloc_fails)
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