//@	{"target":{"name":"callback_binding.test"}}

#include "./callback_binding.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct my_test_class
	{
	public:
		my_test_class():expected_this{this}{}

		my_test_class* expected_this;

		int times(int a, int b)
		{
			EXPECT_EQ(this, expected_this);
			return a*b;

		}

		int minus(this my_test_class& self, int a, int b)
		{
			EXPECT_EQ(&self, self.expected_this);
			return a - b;
		}
	};
}

TESTCASE(Pipe_utils_callback_binding_bind_and_call)
{
	my_test_class foo{};
	Pipe::utils::callback_binding times{foo, Pipe::utils::make_type<&my_test_class::times>{}};
	EXPECT_EQ(times(3, 2), 6);

	Pipe::utils::callback_binding minus{foo, Pipe::utils::make_type<&my_test_class::minus>{}};
	EXPECT_EQ(minus(3, 2), 1);

	auto const alt_minus = Pipe::utils::make_binding<&my_test_class::minus>(foo);
	EXPECT_EQ(alt_minus(3, 8), -5);
}
