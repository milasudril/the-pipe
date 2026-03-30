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