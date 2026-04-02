//@	{"target":{"name":"./worker_sync.test"}}

#include "./worker_sync.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <array>
#include <testfwk/testfwk.hpp>

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
	static constexpr std::array<std::byte, 29> input_bytes{
	// Transaction id
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

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
	EXPECT_EQ(res, 29 - 19);
	EXPECT_EQ(decoder.completed(), true);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	auto const result = decoder.get_value();
	EXPECT_EQ(result.transaction_id, 3);
	EXPECT_EQ(result.server_portname.size(), 13);
	EXPECT_EQ(result.server_portname, "Hello, World!");
}

TESTCASE(Pipe_worker_sync_decode_port_activity_subscription_request_for_full_elements)
{
	static constexpr std::array<std::byte, 29> input_bytes{
	// Transaction id
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

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
	auto res = decoder.decode(std::span{std::begin(buffer), 16});
	EXPECT_EQ(res, 16);
	EXPECT_EQ(decoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	res = decoder.decode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 29 - 16);
	EXPECT_EQ(decoder.completed(), true);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};

	auto const result = decoder.get_value();
	EXPECT_EQ(result.transaction_id, 3);
	EXPECT_EQ(result.server_portname.size(), 13);
	EXPECT_EQ(result.server_portname, "Hello, World!");
}

TESTCASE(Pipe_worker_sync_decode_port_activity_subscription_request_junk_after_data)
{
	static constexpr std::array<std::byte, 127> input_bytes{
	// Transaction id
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),
		static_cast<std::byte>(0x00),

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
	EXPECT_EQ(res, 29);
	EXPECT_EQ(decoder.completed(), true);

	auto const result = std::move(decoder.get_value());
	EXPECT_EQ(result.transaction_id, 3);
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
			.transaction_id = 0x0102030405060708,
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
	EXPECT_EQ(res, 29 - 19);
	EXPECT_EQ(encoder.completed(), true);
	
	std::array<std::byte, 127> expected_result{
			// Transaction id
		static_cast<std::byte>(0x08),
		static_cast<std::byte>(0x07),
		static_cast<std::byte>(0x06),
		static_cast<std::byte>(0x05),
		static_cast<std::byte>(0x04),
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x02),
		static_cast<std::byte>(0x01),

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
			.transaction_id = 0x0102030405060708,
			.message = "Hello, World!"
		}
	};
	EXPECT_EQ(encoder.completed(), false);
	
	std::array<std::byte, 127> output_bytes{};
	std::span buffer{std::begin(output_bytes), std::end(output_bytes)};
	
	auto res = encoder.encode(std::span{std::begin(buffer), 16});
	EXPECT_EQ(res, 16);
	EXPECT_EQ(encoder.completed(), false);
	buffer = std::span{std::begin(buffer) + res, std::end(buffer)};
	
	res = encoder.encode(std::span{std::begin(buffer), std::end(buffer)});
	EXPECT_EQ(res, 13);
	EXPECT_EQ(encoder.completed(), true);
	
	std::array<std::byte, 127> expected_result{
			// Transaction id
		static_cast<std::byte>(0x08),
		static_cast<std::byte>(0x07),
		static_cast<std::byte>(0x06),
		static_cast<std::byte>(0x05),
		static_cast<std::byte>(0x04),
		static_cast<std::byte>(0x03),
		static_cast<std::byte>(0x02),
		static_cast<std::byte>(0x01),

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
