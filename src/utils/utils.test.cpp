//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"

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
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
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
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
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
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
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
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_consecutive_boundary_points_not_included)
{
	std::array const vals{1u, 2u, 5u, 6u};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{0u, 0u},
		Pipe::utils::inclusive_integral_range{3u, 4u},
		Pipe::utils::inclusive_integral_range{7u, 0xffff'ffffu},
	};
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_consecutive_boundary_points_included)
{
	std::array const vals{0u, 1u, 3u, 4u, 0xffff'fffeu, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{2u, 2u},
		Pipe::utils::inclusive_integral_range{5u, 0xffff'fffdu}
	};
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
}

TESTCASE(Pipe_utils_for_each_disjoint_segment_vals_outside_range)
{
	std::array const vals{0u, 1u, 2u, 3u, 6u, 0xffff'fffeu, 0xffff'ffffu};
	std::array const expected_results{
		Pipe::utils::inclusive_integral_range{4u, 5u},
		Pipe::utils::inclusive_integral_range{7u, 16u}
	};
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 16u
		},
		std::span{std::begin(vals), std::end(vals)},
		[k = 0](auto range, auto const& expected_results) mutable {
			EXPECT_EQ(range, expected_results[k]);
			++k;
		},
		expected_results
	);
}