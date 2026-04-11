//@	{"target":{"name":"./worker_sync.test"}}

#include "./worker_sync.hpp"
#include "testfwk/validation.hpp"

#include <array>
#include <cstddef>
#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_sync_transaction_id_valid_after_wrap_around)
{
	Pipe::worker_sync::transaction_id tx_id{0xffff'ffff'ffff'ffff};
	EXPECT_EQ(tx_id.is_valid(), true);
	EXPECT_EQ(tx_id.value(), 0x7fff'ffff'ffff'ffff);

	auto const next_id = tx_id.next();
	EXPECT_EQ(next_id, Pipe::worker_sync::transaction_id{0xffff'ffff'ffff'ffff});
	EXPECT_EQ(tx_id.value(), 0);
	EXPECT_EQ(tx_id.is_valid(), true);
}

TESTCASE(Pipe_worker_sync_transaction_id_invalid_by_defualt)
{
	Pipe::worker_sync::transaction_id tx_id{};
	EXPECT_EQ(tx_id.is_valid(), false);
}

TESTCASE(Pipe_worker_sync_decode_trivially_copyable_type)
{
	Pipe::worker_sync::decoder<uint64_t> decoder{};

	{
		static constexpr std::array<std::byte, 3> input_bytes{
			static_cast<std::byte>(0x01),
			static_cast<std::byte>(0x02),
			static_cast<std::byte>(0x03)
		};

		auto const res = decoder.decode(input_bytes);
		EXPECT_EQ(res, 3);
		EXPECT_EQ(decoder.completed(), false);
	}

	{
		static constexpr std::array<std::byte, 5> input_bytes{
			static_cast<std::byte>(0x04),
			static_cast<std::byte>(0x05),
			static_cast<std::byte>(0x06),
			static_cast<std::byte>(0x07),
			static_cast<std::byte>(0x08)
		};

		auto const res = decoder.decode(input_bytes);
		EXPECT_EQ(res, 5);
		EXPECT_EQ(decoder.completed(), true);
	}

	EXPECT_EQ(decoder.get_value(), 0x0807060504030201);
}

TESTCASE(Pipe_worker_sync_decode_trivially_copyable_type_junk_after_data)
{
	Pipe::worker_sync::decoder<uint64_t> decoder{};

	static constexpr std::array<std::byte, 127> input_bytes{
		static_cast<std::byte>(0x01),
		static_cast<std::byte>(0x02),
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x04),
		static_cast<std::byte>(0x05),
		static_cast<std::byte>(0x06),
		static_cast<std::byte>(0x07),
		static_cast<std::byte>(0x08)
	};

	auto res = decoder.decode(input_bytes);
	EXPECT_EQ(res, 8);
	EXPECT_EQ(decoder.completed(), true);
	EXPECT_EQ(decoder.get_value(), 0x0807060504030201);

	std::span remaining{std::begin(input_bytes) + res, std::end(input_bytes)};
	res = decoder.decode(remaining);
	EXPECT_EQ(res, 0);
	EXPECT_EQ(decoder.completed(), true);
	EXPECT_EQ(decoder.get_value(), 0x0807060504030201);
}

TESTCASE(Pipe_worker_sync_encode_trivially_copyable_type)
{
	Pipe::worker_sync::encoder<uint64_t> encoder{static_cast<uint64_t>(0x0807060504030201)};
	EXPECT_EQ(encoder.completed(), false);

	std::array<std::byte, 8> output_bytes{};

	std::span output_range{std::begin(output_bytes), 3};
	auto res = encoder.encode(output_range);
	EXPECT_EQ(res, 3);
	EXPECT_EQ(encoder.completed(), false);

	output_range = std::span{std::begin(output_bytes) + res, std::end(output_bytes)};
	res = encoder.encode(output_range);
	EXPECT_EQ(res, 5);
	EXPECT_EQ(encoder.completed(), true);

	static constexpr std::array<std::byte, 8> expected_output{
		static_cast<std::byte>(0x01),
		static_cast<std::byte>(0x02),
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x04),
		static_cast<std::byte>(0x05),
		static_cast<std::byte>(0x06),
		static_cast<std::byte>(0x07),
		static_cast<std::byte>(0x08)
	};

	EXPECT_EQ(output_bytes, expected_output);
}

TESTCASE(Pipe_worker_sync_decode_port_activity_subscription_request_for_partial_elements)
{
	static constexpr std::array<std::byte, 21> input_bytes{

	// String length
		static_cast<std::byte>(13),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

	// Actual string
		static_cast<std::byte>('H'),
		static_cast<std::byte>('e'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>(','),
		static_cast<std::byte>(' '),
		static_cast<std::byte>('W'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>('r'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('d'),
		static_cast<std::byte>('!')
	};

	std::span buffer{std::begin(input_bytes), std::end(input_bytes)};
	Pipe::worker_sync::decoder<Pipe::worker_sync::port_activity_subscription_request> decoder{};
	auto res = decoder.decode(std::span{std::begin(buffer), 11});
	EXPECT_EQ(res, 11);
	EXPECT_EQ(decoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = decoder.decode(std::span{std::begin(buffer), 8});
	EXPECT_EQ(res, 8);
	EXPECT_EQ(decoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = decoder.decode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 21 - 19);
	EXPECT_EQ(decoder.completed(), true);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	auto const result = decoder.get_value();
	EXPECT_EQ(result.server_portname.size(), 13);
	EXPECT_EQ(result.server_portname, "Hello, World!");
}

TESTCASE(Pipe_worker_sync_decode_port_activity_subscription_request_for_full_elements)
{
	static constexpr std::array<std::byte, 21> input_bytes{
	// String length
		static_cast<std::byte>(13),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

	// Actual string
		static_cast<std::byte>('H'),
		static_cast<std::byte>('e'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>(','),
		static_cast<std::byte>(' '),
		static_cast<std::byte>('W'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>('r'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('d'),
		static_cast<std::byte>('!')
	};

	std::span buffer{std::begin(input_bytes), std::end(input_bytes)};
	Pipe::worker_sync::decoder<Pipe::worker_sync::port_activity_subscription_request> decoder{};
	auto res = decoder.decode(std::span{std::begin(buffer), 8});
	EXPECT_EQ(res, 8);
	EXPECT_EQ(decoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = decoder.decode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 21 - 8);
	EXPECT_EQ(decoder.completed(), true);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	auto const result = decoder.get_value();
	EXPECT_EQ(result.server_portname.size(), 13);
	EXPECT_EQ(result.server_portname, "Hello, World!");
}

TESTCASE(Pipe_worker_sync_decode_port_activity_subscription_request_junk_after_data)
{
	static constexpr std::array<std::byte, 127> input_bytes{
	// String length
		static_cast<std::byte>(13),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

	// Actual string
		static_cast<std::byte>('H'),
		static_cast<std::byte>('e'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>(','),
		static_cast<std::byte>(' '),
		static_cast<std::byte>('W'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>('r'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('d'),
		static_cast<std::byte>('!')
	};

	std::span buffer{std::begin(input_bytes), std::end(input_bytes)};
	Pipe::worker_sync::decoder<Pipe::worker_sync::port_activity_subscription_request> decoder{};
	auto res = decoder.decode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 21);
	EXPECT_EQ(decoder.completed(), true);

	auto const result = std::move(decoder.get_value());
	EXPECT_EQ(result.server_portname.size(), 13);
	EXPECT_EQ(result.server_portname, "Hello, World!");

	buffer = std::span{std::begin(input_bytes) + res, std::end(input_bytes)};
	res = decoder.decode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 0);
}

TESTCASE(Pipe_worker_sync_encode_error_response_partial_elements)
{
	Pipe::worker_sync::encoder<Pipe::worker_sync::error_response> encoder{
		Pipe::worker_sync::error_response{
			.message = "Hello, World!"
		}
	};
	EXPECT_EQ(encoder.completed(), false);

	std::array<std::byte, 127> output_bytes{};
	std::span buffer{std::begin(output_bytes), std::end(output_bytes)};

	auto res = encoder.encode(std::span{std::begin(buffer), 11});
	EXPECT_EQ(res, 11);
	EXPECT_EQ(encoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = encoder.encode(std::span{std::begin(buffer), 8});
	EXPECT_EQ(res, 8);
	EXPECT_EQ(encoder.completed(), false);

	res = encoder.encode(std::span{std::begin(buffer) + res, std::end(buffer)});
	EXPECT_EQ(res, 21 - 19);
	EXPECT_EQ(encoder.completed(), true);

	std::array<std::byte, 127> expected_result{
	// String length
		static_cast<std::byte>(13),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

	// Actual string
		static_cast<std::byte>('H'),
		static_cast<std::byte>('e'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>(','),
		static_cast<std::byte>(' '),
		static_cast<std::byte>('W'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>('r'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('d'),
		static_cast<std::byte>('!')
	};

	EXPECT_EQ(output_bytes, expected_result);
}

TESTCASE(Pipe_worker_sync_encode_error_response_full_elements)
{
	Pipe::worker_sync::encoder<Pipe::worker_sync::error_response> encoder{
		Pipe::worker_sync::error_response{
			.message = "Hello, World!"
		}
	};
	EXPECT_EQ(encoder.completed(), false);

	std::array<std::byte, 127> output_bytes{};
	std::span buffer{std::begin(output_bytes), std::end(output_bytes)};

	auto res = encoder.encode(std::span{std::begin(buffer), 8});
	EXPECT_EQ(res, 8);
	EXPECT_EQ(encoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = encoder.encode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 13);
	EXPECT_EQ(encoder.completed(), true);

	std::array<std::byte, 127> expected_result{
	// String length
		static_cast<std::byte>(13),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

	// Actual string
		static_cast<std::byte>('H'),
		static_cast<std::byte>('e'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>(','),
		static_cast<std::byte>(' '),
		static_cast<std::byte>('W'),
		static_cast<std::byte>('o'),
		static_cast<std::byte>('r'),
		static_cast<std::byte>('l'),
		static_cast<std::byte>('d'),
		static_cast<std::byte>('!')
	};

	EXPECT_EQ(output_bytes, expected_result);
}

namespace
{
	class test_decoder:public Pipe::worker_sync::decoder_base
	{
	public:
		bool completed() const
		{
			return is_completed.value();
		}

		size_t decode(std::span<std::byte const> val)
		{
			EXPECT_EQ(std::begin(val), std::begin(expected_range.value()));
			return return_size.value();
		}

		int get_value() const
		{
			return return_value.value();
		}

		std::optional<bool> is_completed;
		std::optional<std::span<std::byte const>> expected_range;
		std::optional<size_t> return_size;
		std::optional<int> return_value;
	};
}

TESTCASE(Pipe_worker_sync_decoder_base_decode_not_completed)
{
	test_decoder decoder{};
	decoder.is_completed = false;
	std::array<std::byte, 16> data{};
	decoder.expected_range = data;
	decoder.return_size = 13;
	auto const res = decoder.decode_and_dispatch(
		data,
		[](auto&&...){
			throw std::runtime_error{"Unexpected call 1"};
		},
		[](auto&&...){
			throw std::runtime_error{"Unexpected call 2"};
		}
	);
	EXPECT_EQ(res, 13);
}

TESTCASE(Pipe_worker_sync_decoder_base_decode_completed_callback_throws)
{
	test_decoder decoder{};
	decoder.is_completed = true;
	std::array<std::byte, 16> data{};
	decoder.expected_range = data;
	decoder.return_size = 13;
	decoder.return_value = 465;
	std::optional<Pipe::worker_sync::error_response> response;
	auto const res = decoder.decode_and_dispatch(
		data,
		[](int val){
			EXPECT_EQ(val, 465);
			throw std::runtime_error{"Callback failed"};
		},
		[&response]<class T>(T&& val){
			response = std::forward<T>(val);
		}
	);
	EXPECT_EQ(res, 13);
	REQUIRE_EQ(response.has_value(), true);
	EXPECT_EQ(response.value().message, "Callback failed");
}

TESTCASE(Pipe_worker_sync_decoder_base_decode_completed_callback_succeeds)
{
	test_decoder decoder{};
	decoder.is_completed = true;
	std::array<std::byte, 16> data{};
	decoder.expected_range = data;
	decoder.return_size = 13;
	decoder.return_value = 465;
	std::optional<Pipe::worker_sync::error_response> response;
	auto const res = decoder.decode_and_dispatch(
		data,
		[](int val){
			EXPECT_EQ(val, 465);
		},
		[&response]<class T>(T&& val){
			response = std::forward<T>(val);
		}
	);
	EXPECT_EQ(res, 13);
	EXPECT_EQ(response.has_value(), false);
}

TESTCASE(Pipe_worker_sync_port_activity_subscription_id)
{
	Pipe::worker_sync::port_activity_subscription_id id{};
	EXPECT_EQ(id.value(), 0);
	auto const id_copy = id;
	auto const next = id.next();
	EXPECT_EQ(next, id_copy);
	EXPECT_EQ(id.value(), next.value() + 1);

	EXPECT_EQ(
		std::hash<Pipe::worker_sync::port_activity_subscription_id>{}(Pipe::worker_sync::port_activity_subscription_id{254}),
		std::hash<uint64_t>{}(254)
	);
}
