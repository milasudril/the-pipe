//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_utils_random_printable_ascii_string)
{
	auto const string = Pipe::utils::random_printable_ascii_string(16);
	EXPECT_EQ(std::size(string), 16);
	printf("%s\n", string.c_str());
}

TESTCASE(Pipe_utils_byte_count_to_printable_ascii_string_length)
{
	EXPECT_EQ(Pipe::utils::byte_count_to_printable_ascii_string_length(16), 20);
	static_assert(Pipe::utils::num_chars_16_bytes == 20);
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_points_not_included)
{
	std::array const vals{5u, 9u, 12u, 15u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{0u, 4u},
		Pipe::utils::inclusive_integral_range{6u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'ffffu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_points_not_included_start_nonzero)
{
	std::array const vals{5u, 9u, 12u, 15u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{3u, 4u},
		Pipe::utils::inclusive_integral_range{6u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'ffffu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_points_not_included_start_nonzero_split_directly_after)
{
	std::array const vals{4u, 9u, 12u, 15u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{3u, 3u},
		Pipe::utils::inclusive_integral_range{5u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'ffffu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_other_testcase)
{
	std::array const vals{8u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{3u, 7u},
		Pipe::utils::inclusive_integral_range{9u, 0xffff'ffffu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_start_at_included)
{
	std::array const vals{0u, 5u, 9u, 12u, 15u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{1u, 4u},
		Pipe::utils::inclusive_integral_range{6u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'ffffu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_stop_at_included)
{
	std::array const vals{5u, 9u, 12u, 15u, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{0u, 4u},
		Pipe::utils::inclusive_integral_range{6u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'fffeu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_boundary_points_included)
{
	std::array const vals{0u, 5u, 9u, 12u, 15u, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{1u, 4u},
		Pipe::utils::inclusive_integral_range{6u, 8u},
		Pipe::utils::inclusive_integral_range{10u, 11u},
		Pipe::utils::inclusive_integral_range{13u, 14u},
		Pipe::utils::inclusive_integral_range{16u, 0xffff'fffeu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_consecutive_boundary_points_not_included)
{
	std::array const vals{1u, 2u, 5u, 6u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{0u, 0u},
		Pipe::utils::inclusive_integral_range{3u, 4u},
		Pipe::utils::inclusive_integral_range{7u, 0xffff'ffffu},
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_consecutive_boundary_points_included)
{
	std::array const vals{0u, 1u, 3u, 4u, 0xffff'fffeu, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{2u, 2u},
		Pipe::utils::inclusive_integral_range{5u, 0xffff'fffdu}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_vals_outside_range)
{
	std::array const vals{0u, 1u, 2u, 3u, 6u, 0xffff'fffeu, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{4u, 5u},
		Pipe::utils::inclusive_integral_range{7u, 16u}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 16u
		},
		Pipe::utils::flat_set{std::begin(vals), std::end(vals)},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_empty_set_returns_full_range)
{
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{3u, 16u}
	};
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 16u
		},
		Pipe::utils::immutable_flat_set<unsigned int>{},
		[&k](auto range, auto const& expected_results) {
			REQUIRE_LT(k, std::size(expected_results));
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
	EXPECT_EQ(k, std::size(expected_results));
}