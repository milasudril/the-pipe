//@	{"target":{"name":"file_descriptor.test"}}

#include "./file_descriptor.hpp"

#include <concepts>
#include <testfwk/testfwk.hpp>
#include <fcntl.h>
#include <functional>

namespace
{
	struct convert_from
	{};

	struct convert_to
	{};
}

template<>
struct prog::utils::enabled_fd_conversions<convert_from>
{
	static consteval void supports(convert_to){}
};

// Static checks
static_assert(std::is_same_v<prog::utils::tagged_file_descriptor_ref<int>::tag_type, int>);
static_assert(std::equality_comparable<prog::utils::tagged_file_descriptor_ref<int>>);
static_assert(std::equality_comparable_with<prog::utils::tagged_file_descriptor_ref<int>, nullptr_t>);
static_assert(std::is_convertible_v<prog::utils::tagged_file_descriptor_ref<int>, bool>);
static_assert(
	std::is_convertible_v<
		prog::utils::tagged_file_descriptor_ref<convert_from>,
		prog::utils::tagged_file_descriptor_ref<convert_to>
	>
);
static_assert(
	!std::is_convertible_v<
		prog::utils::tagged_file_descriptor_ref<convert_from>,
		prog::utils::tagged_file_descriptor_ref<int>
	>
);

TESTCASE(prog_utils_tagged_file_descriptor_ref_create_id)
{
	prog::utils::tagged_file_descriptor_ref<int> fd{34};
	EXPECT_EQ(fd.native_handle(), 34);
	EXPECT_EQ(fd.is_valid(), true);
}

TESTCASE(prog_utils_tagged_file_descriptor_ref_create_default)
{
	prog::utils::tagged_file_descriptor_ref<int> fd;
	EXPECT_EQ(fd.native_handle(), -1);
	EXPECT_EQ(fd.is_valid(), false);
}

TESTCASE(prog_utils_tagged_file_descriptor_ref_create_from_nullptr)
{
	prog::utils::tagged_file_descriptor_ref<int> fd{nullptr};
	EXPECT_EQ(fd.native_handle(), -1);
	EXPECT_EQ(fd.is_valid(), false);
}

TESTCASE(prog_utils_tagged_file_descriptor_ref_convert)
{
	prog::utils::tagged_file_descriptor_ref<convert_from> from{213};
	prog::utils::tagged_file_descriptor_ref<convert_to> to = from;
	EXPECT_EQ(from.native_handle(), 213);
	EXPECT_EQ(to.native_handle(), 213);
}

TESTCASE(prog_utils_tagged_file_descriptor_create)
{
	prog::utils::tagged_file_descriptor<int> fd{open("/dev/null", O_RDONLY)};
	EXPECT_NE(fd, nullptr);
	fd.reset();
	EXPECT_EQ(fd, nullptr);
}

namespace
{
	struct foo{};
}

template<>
struct prog::utils::file_descriptor_deleter<foo>
{
	using pointer = tagged_file_descriptor_ref<foo>;
	std::move_only_function<void(int)> close_stub;

	void operator()(tagged_file_descriptor_ref<foo> x)
	{
		close_stub(x.native_handle());
	}
};

TESTCASE(prog_utils_tagged_file_descriptor_create_specialized_delter)
{
	prog::utils::tagged_file_descriptor<foo> fd{124};
	size_t delete_callcount = 0;
	fd.get_deleter().close_stub = [
		expected_fd = fd.get().native_handle(),
		&delete_callcount
	](int fd){
		EXPECT_EQ(fd, expected_fd);
		++delete_callcount;
	};
	EXPECT_NE(fd, nullptr);
	fd.reset();
	EXPECT_EQ(fd, nullptr);
	EXPECT_EQ(delete_callcount, 1);
}