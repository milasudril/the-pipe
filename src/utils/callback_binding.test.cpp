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

		int times(this my_test_class& self, int a, int b)
		{
			EXPECT_EQ(&self, self.expected_this);
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
	Pipe::utils::callback_binding times{foo, &my_test_class::times};
	EXPECT_EQ(times(3, 2), 6);

	Pipe::utils::callback_binding minus{foo, &my_test_class::minus};
	EXPECT_EQ(minus(3, 2), 1);
}
