//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>
#include <flat_map>
#include <string>

TESTCASE(Pipe_utils_random_printable_ascii_string)
{
	auto const string = Pipe::utils::random_printable_ascii_string(16);
	EXPECT_EQ(std::size(string), 16);
	printf("%s\n", string.c_str());
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 0u,
			.stop_at = 0xffff'ffffu
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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
	std::flat_set<uint32_t> set(std::begin(vals), std::end(vals));
	size_t k = 0;
	for_each_disjoint_segment(
		Pipe::utils::inclusive_integral_range{
			.start_at = 3u,
			.stop_at = 16u
		},
		Pipe::utils::immutable_flat_set{std::cref(set)},
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

TESTCASE(Pipe_utils_compute_struct_info)
{
	{
		auto const result = compute_struct_info(
			std::array{
				Pipe::utils::struct_field_info{.size = 1, .alignment = 1},
				Pipe::utils::struct_field_info{.size = 4, .alignment = 4},
				Pipe::utils::struct_field_info{.size = 2, .alignment = 2}
			}
		);

		EXPECT_EQ(result.offsets[0], 0);
		EXPECT_EQ(result.offsets[1], 4);
		EXPECT_EQ(result.offsets[2], 8);
		EXPECT_EQ(result.total_size, 12);
	}

	{
		auto const result = compute_struct_info(
			std::array{
				Pipe::utils::struct_field_info{.size = 8, .alignment = 8},
				Pipe::utils::struct_field_info{.size = 4, .alignment = 4},
				Pipe::utils::struct_field_info{.size = 1, .alignment = 1}
			}
		);

		EXPECT_EQ(result.offsets[0], 0);
		EXPECT_EQ(result.offsets[1], 8);
		EXPECT_EQ(result.offsets[2], 12);
		EXPECT_EQ(result.total_size, 16);
	}

	{
		auto const result = compute_struct_info(
			std::array{
				Pipe::utils::struct_field_info{.size = 1, .alignment = 1},
				Pipe::utils::struct_field_info{.size = 8, .alignment = 8},
				Pipe::utils::struct_field_info{.size = 1, .alignment = 1}
			}
		);

		EXPECT_EQ(result.offsets[0], 0);
		EXPECT_EQ(result.offsets[1], 8);
		EXPECT_EQ(result.offsets[2], 16);
		EXPECT_EQ(result.total_size, 24);
	}

	{
		auto const result = compute_struct_info(
			std::array{
				Pipe::utils::struct_field_info{.size = 10, .alignment = 1},
				Pipe::utils::struct_field_info{.size = 4, .alignment = 4},
				Pipe::utils::struct_field_info{.size = 2, .alignment = 2}
			}
		);

		EXPECT_EQ(result.offsets[0], 0);
		EXPECT_EQ(result.offsets[1], 12);
		EXPECT_EQ(result.offsets[2], 16);
		EXPECT_EQ(result.total_size, 20);
	}

	{
		auto const result = compute_struct_info(
			std::array{
				Pipe::utils::struct_field_info{.size = 4, .alignment = 4},
				Pipe::utils::struct_field_info{.size = 4, .alignment = 16}
			}
		);

		EXPECT_EQ(result.offsets[0], 0);
		EXPECT_EQ(result.offsets[1], 16);
		EXPECT_EQ(result.total_size, 32);
	}
}

TESTCASE(Pipe_utils_write_buffer_putchars)
{
	size_t flush_count = 0;
	std::string written_data;
	{
		auto buffer = Pipe::utils::make_write_buffer<16>(
			[&flush_count, &written_data](std::span<char const> buffer) {
				switch(flush_count)
				{
					case 0:
						EXPECT_EQ(std::size(buffer), 16);
						break;
					case 1:
						EXPECT_EQ(std::size(buffer), 16);
						break;
					default:
						EXPECT_EQ(std::size(buffer), 0);
				}

				written_data.append(std::begin(buffer), std::end(buffer));
				++flush_count;
			}
		);

		for(size_t k = 0; k != 32; ++k)
		{ buffer.putchar(static_cast<char>(k + 32)); }
	}

	EXPECT_EQ(flush_count, 2);
	EXPECT_EQ(std::size(written_data), 32);
	EXPECT_EQ(written_data, " !\"#$%&'()*+,-./0123456789:;<=>?");
}


TESTCASE(Pipe_utils_write_buffer_putchars_flush_manually)
{
	size_t flush_count = 0;
	std::string written_data;
	{
		auto buffer = Pipe::utils::make_write_buffer<16>(
			[&flush_count, &written_data](std::span<char const> buffer) {
				switch(flush_count)
				{
					case 0:
						EXPECT_EQ(std::size(buffer), 16);
						break;
					case 1:
						EXPECT_EQ(std::size(buffer), 8);
						break;
					default:
						EXPECT_EQ(std::size(buffer), 0);
				}

				written_data.append(std::begin(buffer), std::end(buffer));
				++flush_count;
			}
		);

		for(size_t k = 0; k != 24; ++k)
		{ buffer.putchar(static_cast<char>(k + 32)); }

		buffer.flush();
		EXPECT_EQ(flush_count, 2);
		EXPECT_EQ(std::size(written_data), 24);
		EXPECT_EQ(written_data, " !\"#$%&'()*+,-./01234567");
	}
	EXPECT_EQ(flush_count, 2);
	EXPECT_EQ(std::size(written_data), 24);
	EXPECT_EQ(written_data, " !\"#$%&'()*+,-./01234567");
}

TESTCASE(Pipe_utils_write_buffer_puts)
{
	size_t flush_count = 0;
	std::string written_data;
	{
		auto buffer = Pipe::utils::make_write_buffer<16>(
			[&flush_count, &written_data](std::span<char const> buffer) {
				switch(flush_count)
				{
					case 0:
						EXPECT_EQ(std::size(buffer), 16);
						break;
					case 1:
						EXPECT_EQ(std::size(buffer), 8);
						break;
					default:
						EXPECT_EQ(std::size(buffer), 0);
				}

				written_data.append(std::begin(buffer), std::end(buffer));
				++flush_count;
			}
		);

		buffer.puts(" !\"#$%&'()*+,-./01234567");

		EXPECT_EQ(flush_count, 1);
		EXPECT_EQ(std::size(written_data), 16);
		EXPECT_EQ(written_data, " !\"#$%&'()*+,-./");
	}
	EXPECT_EQ(flush_count, 2);
	EXPECT_EQ(std::size(written_data), 24);
	EXPECT_EQ(written_data, " !\"#$%&'()*+,-./01234567");
}

TESTCASE(Pipe_utils_pair_remove_if)
{
	std::flat_map<std::string, std::unique_ptr<int>> vals;

	vals.insert({"foo", std::make_unique<int>(1)});
	vals.insert({"bar", std::make_unique<int>(2)});
	vals.insert({"bajs", std::make_unique<int>(2)});
	vals.insert({"bulle", std::make_unique<int>(2)});
	vals.insert({"kaka", std::make_unique<int>(3)});

	auto [keys, values] = std::move(vals).extract();
	std::ranges::zip_view remove_from{keys, values};
	auto const removed_part = Pipe::utils::pair_remove_if(
		remove_from,
		[](auto const& item) {
			return *item.second == 2;
		}
	);
	EXPECT_EQ(std::size(removed_part), 3);

	std::ranges::subrange const remaining{std::begin(remove_from), std::begin(removed_part)};
	EXPECT_EQ(std::size(remaining), 2);
	EXPECT_EQ(std::get<0>(remaining[0]), "foo");
	EXPECT_EQ(std::get<0>(remaining[1]), "kaka");
}


TESTCASE(Pipe_utils_flatmap_erase_if)
{
	std::flat_map<std::string, std::unique_ptr<int>> vals;

	vals.insert({"foo", std::make_unique<int>(1)});
	vals.insert({"bar", std::make_unique<int>(2)});
	vals.insert({"bajs", std::make_unique<int>(2)});
	vals.insert({"bulle", std::make_unique<int>(2)});
	vals.insert({"kaka", std::make_unique<int>(3)});

	Pipe::utils::flatmap_erase_if(
		vals,
		[](auto const& item) {
			return *item.second == 2;
		}
	);

	EXPECT_EQ(std::size(vals), 2);
	EXPECT_EQ(*vals.at("foo"), 1);
	EXPECT_EQ(*vals.at("kaka"), 3);
}