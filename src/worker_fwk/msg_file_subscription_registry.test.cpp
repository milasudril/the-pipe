//@	{"target":{"name":"msg_file_subscription_registry.test"}}

#include "./msg_file_subscription_registry.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

#include <cstdio>
#include <testfwk/testfwk.hpp>
#include <deque>

namespace
{
	struct test_port_collection
	{
		auto get_msg_file_output_ports()
		{
			return std::array{
				std::pair{
					Pipe::worker_fwk::port_id{0},
					Pipe::utils::bind_member_function<&test_port_collection::port_0_ready>(*this)
				},
				std::pair{
					Pipe::worker_fwk::port_id{1},
					Pipe::utils::bind_member_function<&test_port_collection::port_1_ready>(*this)
				},
				std::pair{
					Pipe::worker_fwk::port_id{2},
					Pipe::utils::bind_member_function<&test_port_collection::port_2_ready>(*this)
				},
				std::pair{
					Pipe::worker_fwk::port_id{3},
					Pipe::utils::bind_member_function<&test_port_collection::port_3_ready>(*this)
				},
				std::pair{
					Pipe::worker_fwk::port_id{4},
					Pipe::utils::bind_member_function<&test_port_collection::port_4_ready>(*this)
				}
			};
		}

		Pipe::worker_fwk::port_id get_port_id(std::string const& port_name)
		{
			static constexpr std::array port_names{
				"port_0",
				"port_1",
				"port_2",
				"port_3",
				"port_4",
				"port_5"
			};

			auto const i = std::ranges::find(port_names, port_name);
			if(i == std::end(port_names))
			{  throw std::runtime_error{"No such port exists"}; }

			return Pipe::worker_fwk::port_id{static_cast<size_t>(i - std::begin(port_names))};
		}

		bool expect_port_0_ready = false;
		bool expect_port_0_ready_exception = false;
		void port_0_ready()
		{
			if(expect_port_0_ready_exception)
			{
				expect_port_0_ready_exception = false;
				throw std::runtime_error{"Something went wrong"};
			}

			EXPECT_EQ(expect_port_0_ready, true);
			expect_port_0_ready = false;
		}

		bool expect_port_1_ready = false;
		void port_1_ready()
		{
			EXPECT_EQ(expect_port_1_ready, true);
			expect_port_1_ready = false;
		}

		bool expect_port_2_ready = false;
		void port_2_ready()
		{
			EXPECT_EQ(expect_port_2_ready, true);
			expect_port_2_ready = false;
		}

		bool expect_port_3_ready = false;
		void port_3_ready()
		{
			EXPECT_EQ(expect_port_3_ready, true);
			expect_port_3_ready = false;
		}

		bool expect_port_4_ready = false;
		void port_4_ready()
		{
			EXPECT_EQ(expect_port_4_ready, true);
			expect_port_4_ready = false;
		}

		~test_port_collection()
		{
			EXPECT_EQ(expect_port_0_ready, false);
			EXPECT_EQ(expect_port_1_ready, false);
			EXPECT_EQ(expect_port_2_ready, false);
			EXPECT_EQ(expect_port_3_ready, false);
			EXPECT_EQ(expect_port_4_ready, false);
		}
	};

	struct my_activity_subscriber
	{
		std::deque<Pipe::worker_sync::port_activity_subscription_id> expected_ids;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			REQUIRE_NE(expected_ids.empty(), true);
			EXPECT_EQ(id, expected_ids.front());
			expected_ids.pop_front();
		}

		~my_activity_subscriber()
		{ EXPECT_EQ(expected_ids.empty(), true); }
	};

	alignas(32) std::array<std::byte, 1024*1024> malloc_buffer;

	constinit size_t fail_malloc_no = static_cast<size_t>(-1);
	constinit size_t malloc_count = 0;
	constinit size_t malloc_offset = 0;
}

extern "C"
{
	void* malloc(size_t n)
	{
		Pipe::utils::at_scope_exit _{
			[](){
				++malloc_count;
			}
		};

		if(malloc_count == fail_malloc_no)
		{
			errno = ENOMEM;
			return nullptr;
		}

		static constexpr size_t alignment = 32;
		auto const size_aligned = (n + (alignment - 1)) & ~(alignment - 1);

		if(malloc_offset + size_aligned >= std::size(malloc_buffer))
		{ abort(); }

		auto const retval = std::data(malloc_buffer) + malloc_offset;
		malloc_offset += size_aligned;
		return retval;
	}

	void free(void*)
	{}
}

static_assert(Pipe::worker_fwk::output_port_activity_subscription_registry<Pipe::worker_fwk::msg_file_subscription_registry>);

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_construct_with_port_collection)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	auto const& output_ports = registry.get_msg_file_output_ports();
	EXPECT_EQ(std::size(output_ports), 5);

	auto const& port_0 = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(port_0.get_num_subscriptions(), 0);
	EXPECT_EQ(
		port_0.is_bound_to(
			Pipe::utils::bind_member_function<&test_port_collection::port_0_ready>(port_collection)
		),
		true
	);

	auto const& port_1 = output_ports.at(Pipe::worker_fwk::port_id{1});
	EXPECT_EQ(port_1.get_num_subscriptions(), 0);
	EXPECT_EQ(
		port_1.is_bound_to(
			Pipe::utils::bind_member_function<&test_port_collection::port_1_ready>(port_collection)
		),
		true
	);

	auto const& port_2 = output_ports.at(Pipe::worker_fwk::port_id{2});
	EXPECT_EQ(port_2.get_num_subscriptions(), 0);
	EXPECT_EQ(
		port_2.is_bound_to(
			Pipe::utils::bind_member_function<&test_port_collection::port_2_ready>(port_collection)
		),
		true
	);

	auto const& port_3 = output_ports.at(Pipe::worker_fwk::port_id{3});
	EXPECT_EQ(port_3.get_num_subscriptions(), 0);
	EXPECT_EQ(
		port_3.is_bound_to(
			Pipe::utils::bind_member_function<&test_port_collection::port_3_ready>(port_collection)
		),
		true
	);

	auto const& port_4 = output_ports.at(Pipe::worker_fwk::port_id{4});
	EXPECT_EQ(port_4.get_num_subscriptions(), 0);
	EXPECT_EQ(
		port_4.is_bound_to(
			Pipe::utils::bind_member_function<&test_port_collection::port_4_ready>(port_collection)
		),
		true
	);

	auto const& activity_subscriptions = registry.get_port_acivity_subscriptions();
	EXPECT_EQ(std::size(activity_subscriptions), 0);

	auto const current_subscription_id = registry.get_current_subscription_id();
	EXPECT_EQ(current_subscription_id, Pipe::worker_sync::port_activity_subscription_id{0});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_to_add_subscriber_bad_port_name)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		registry.add_port_activity_subscription(
			"some_unknown_port",
			Pipe::worker_fwk::output_port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"No such port exists"});
	}

	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 0);
	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{0}
	);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_to_add_subscriber_wrong_port_type)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		registry.add_port_activity_subscription(
			"port_5",
			Pipe::worker_fwk::output_port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The given port is not providing a message file"});
	}

	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 0);
	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{0}
	);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_to_add_subcriber_map_insert_fails)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		fail_malloc_no = malloc_count + 1;
		registry.add_port_activity_subscription(
			"port_0",
			Pipe::worker_fwk::output_port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"std::bad_alloc"}); }

	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 0);
	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{0}
	);
	auto const& output_ports = registry.get_msg_file_output_ports();
	auto const& port = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(port.get_num_subscriptions(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_add_subcriber_success)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	port_collection.expect_port_0_ready = true;
	auto const res = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{}
	);

	EXPECT_EQ(res, Pipe::worker_sync::port_activity_subscription_id{0});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_add_subcribers_notify_data_ready)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber_0{};
	my_activity_subscriber subscriber_1{};

	port_collection.expect_port_0_ready = true;
	auto const res_0 = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_0, Pipe::worker_sync::port_activity_subscription_id{0});

	port_collection.expect_port_0_ready = true;
	auto const res_1 = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_1, Pipe::worker_sync::port_activity_subscription_id{1});

	port_collection.expect_port_1_ready = true;
	auto const res_2 = registry.add_port_activity_subscription(
		"port_1",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_2, Pipe::worker_sync::port_activity_subscription_id{2});

	auto const res_3 = registry.add_port_activity_subscription(
		"port_1",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
	EXPECT_EQ(res_3, Pipe::worker_sync::port_activity_subscription_id{3});

	// Same subscriber on one port twice
	subscriber_0.expected_ids.push_back(res_0);
	subscriber_0.expected_ids.push_back(res_1);
	port_collection.expect_port_0_ready = true;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	// Different subscribers on same port
	subscriber_0.expected_ids.push_back(res_2);
	subscriber_1.expected_ids.push_back(res_3);
	port_collection.expect_port_1_ready = true;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{1});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready_subscription_not_found)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	try
	{
		registry.notify_client_ready(
			Pipe::worker_sync::port_activity_subscription_id{324},
			Pipe::worker_fwk::output_port_activity_subscriber_ref{}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_activity_subscriber subscriber_0{};
	my_activity_subscriber subscriber_1{};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	port_collection.expect_port_0_ready = true;
	auto const res_0 = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_0, Pipe::worker_sync::port_activity_subscription_id{0});

	port_collection.expect_port_1_ready = true;
	auto const res_1 = registry.add_port_activity_subscription(
		"port_1",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
	EXPECT_EQ(res_1, Pipe::worker_sync::port_activity_subscription_id{1});

	subscriber_0.expected_ids.push_back(res_0);
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});
	subscriber_1.expected_ids.push_back(res_1);
	registry.notify_data_ready(Pipe::worker_fwk::port_id{1});

	try
	{
		registry.notify_client_ready(
			res_0,
			Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}

	try
	{
		registry.notify_client_ready(
			res_1,
			Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}

	port_collection.expect_port_0_ready = true;
	registry.notify_client_ready(
		res_0,
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);

	port_collection.expect_port_1_ready = true;
	registry.notify_client_ready(
		res_1,
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_port_activity_subscription_subscription_not_found)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	try
	{
		registry.remove_port_activity_subscription(
			Pipe::worker_sync::port_activity_subscription_id{324},
			Pipe::worker_fwk::output_port_activity_subscriber_ref{}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_port_activity_subscription)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_activity_subscriber subscriber_0{};
	my_activity_subscriber subscriber_1{};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	port_collection.expect_port_0_ready = true;
	auto const res_0 = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_0, Pipe::worker_sync::port_activity_subscription_id{0});

	port_collection.expect_port_1_ready = true;
	auto const res_1 = registry.add_port_activity_subscription(
		"port_1",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
	EXPECT_EQ(res_1, Pipe::worker_sync::port_activity_subscription_id{1});

	subscriber_0.expected_ids.push_back(res_0);
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});
	subscriber_1.expected_ids.push_back(res_1);
	registry.notify_data_ready(Pipe::worker_fwk::port_id{1});

	try
	{
		registry.remove_port_activity_subscription(
			res_0,
			Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}

	try
	{
		registry.remove_port_activity_subscription(
			res_1,
			Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}

	port_collection.expect_port_0_ready = true;
	registry.remove_port_activity_subscription(
		res_0,
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);

	port_collection.expect_port_1_ready = true;
	registry.remove_port_activity_subscription(
		res_1,
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_output_port_activity_subscriber)
{
	Pipe::utils::at_scope_exit _{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_activity_subscriber subscriber_0{};
	my_activity_subscriber subscriber_1{};

	test_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	port_collection.expect_port_0_ready = true;
	auto const res_0 = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_0, Pipe::worker_sync::port_activity_subscription_id{0});

	port_collection.expect_port_1_ready = true;
	auto const res_1 = registry.add_port_activity_subscription(
		"port_1",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_1, Pipe::worker_sync::port_activity_subscription_id{1});

	port_collection.expect_port_2_ready = true;
	auto const res_2 = registry.add_port_activity_subscription(
		"port_2",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0}
	);
	EXPECT_EQ(res_2, Pipe::worker_sync::port_activity_subscription_id{2});

	auto const res_3 = registry.add_port_activity_subscription(
		"port_2",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
	EXPECT_EQ(res_3, Pipe::worker_sync::port_activity_subscription_id{3});

	auto const res_4 = registry.add_port_activity_subscription(
		"port_2",
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
	EXPECT_EQ(res_4, Pipe::worker_sync::port_activity_subscription_id{4});

	subscriber_0.expected_ids.push_back(res_0);
	port_collection.expect_port_0_ready = true;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	subscriber_0.expected_ids.push_back(res_1);
	port_collection.expect_port_1_ready = true;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{1});

	subscriber_0.expected_ids.push_back(res_2);
	subscriber_1.expected_ids.push_back(res_3);
	subscriber_1.expected_ids.push_back(res_4);
	port_collection.expect_port_2_ready = true;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{2});

	registry.notify_data_ready(Pipe::worker_fwk::port_id{3});
	registry.notify_data_ready(Pipe::worker_fwk::port_id{4});

	my_activity_subscriber dummy{};
	registry.remove_output_port_activity_subscriber(Pipe::worker_fwk::output_port_activity_subscriber_ref{dummy});
	registry.remove_output_port_activity_subscriber(Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_0});

	port_collection.expect_port_2_ready = true;
	registry.remove_port_activity_subscription(
		res_3,
		Pipe::worker_fwk::output_port_activity_subscriber_ref{subscriber_1}
	);
}