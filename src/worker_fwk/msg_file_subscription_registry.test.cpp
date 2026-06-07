//@	{"target":{"name":"msg_file_subscription_registry.test"}}

#include "./msg_file_subscription_registry.hpp"
#include "src/utils/scope_handling.hpp"

#include <testfwk/testfwk.hpp>

#if 0
namespace
{
	struct my_port_collection
	{
		auto get_msg_file_output_ports()
		{
			return std::array{
				std::pair{
					Pipe::worker_fwk::port_id{0},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_0_ready>(*this)
					}
				},
				std::pair{
					Pipe::worker_fwk::port_id{1},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_1_ready>(*this)
					}
				},
				std::pair{
					Pipe::worker_fwk::port_id{2},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_2_ready>(*this)
					}
				},
				std::pair{
					Pipe::worker_fwk::port_id{3},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_3_ready>(*this)
					}
				},
				std::pair{
					Pipe::worker_fwk::port_id{4},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_4_ready>(*this)
					}
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

		~my_port_collection()
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
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_id;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(id, expected_id);
			expected_id.reset();
		}

		~my_activity_subscriber()
		{ EXPECT_EQ(expected_id.has_value(), false); }
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
		Pipe::utils::at_scope_exit{
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

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_construct_with_port_collection)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	auto const& output_ports = registry.get_msg_file_output_ports();
	EXPECT_EQ(std::size(output_ports), 5);

	auto const& port_0 = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(std::size(port_0.subscriptions), 0);
	EXPECT_EQ(port_0.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_0.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_0.barrier.is_bound_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_0_ready>(port_collection)
		),
		true
	);

	auto const& port_1 = output_ports.at(Pipe::worker_fwk::port_id{1});
	EXPECT_EQ(std::size(port_1.subscriptions), 0);
	EXPECT_EQ(port_1.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_1.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_1.barrier.is_bound_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_1_ready>(port_collection)
		),
		true
	);

	auto const& port_2 = output_ports.at(Pipe::worker_fwk::port_id{2});
	EXPECT_EQ(std::size(port_2.subscriptions), 0);
	EXPECT_EQ(port_2.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_2.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_2.barrier.is_bound_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_2_ready>(port_collection)
		),
		true
	);

	auto const& port_3 = output_ports.at(Pipe::worker_fwk::port_id{3});
	EXPECT_EQ(std::size(port_3.subscriptions), 0);
	EXPECT_EQ(port_3.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_3.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_3.barrier.is_bound_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_3_ready>(port_collection)
		),
		true
	);

	auto const& port_4 = output_ports.at(Pipe::worker_fwk::port_id{4});
	EXPECT_EQ(std::size(port_4.subscriptions), 0);
	EXPECT_EQ(port_4.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_4.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_4.barrier.is_bound_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_4_ready>(port_collection)
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
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		registry.add_port_activity_subscription(
			"some_unknown_port",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
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
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		registry.add_port_activity_subscription(
			"port_5",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
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
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		fail_malloc_no = malloc_count + 1;
		registry.add_port_activity_subscription(
			"port_0",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
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
	EXPECT_EQ(std::size(port.subscriptions), 0);
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port.barrier.get_num_subscribers(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_to_add_subcriber_subscriptions_push_back_fails)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		fail_malloc_no = malloc_count + 3;
		registry.add_port_activity_subscription(
			"port_0",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
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
	EXPECT_EQ(std::size(port.subscriptions), 0);
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port.barrier.get_num_subscribers(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_add_subcriber_callback_throws)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	port_collection.expect_port_0_ready_exception = true;
	try
	{
		registry.add_port_activity_subscription(
			"port_0",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Something went wrong"}); }


	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 0);
	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{0}
	);
	auto const& output_ports = registry.get_msg_file_output_ports();
	auto const& port = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(std::size(port.subscriptions), 0);
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port.barrier.get_num_subscribers(), 0);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_try_add_subcriber_callback_throws_with_ready_clients)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	my_activity_subscriber activity_subscriber{};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::port_activity_subscriber_ref{activity_subscriber}
	);

	// Before signalling that client is ready, signal that data is ready
	activity_subscriber.expected_id = id;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	// Raise an exception when submitting data
	port_collection.expect_port_0_ready_exception = true;
	try
	{
		registry.notify_client_ready(id, Pipe::worker_fwk::port_activity_subscriber_ref{activity_subscriber});
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Something went wrong"}); }

	{
		auto const& output_ports = registry.get_msg_file_output_ports();
		auto const& port = output_ports.at(Pipe::worker_fwk::port_id{0});
		// Still one ready subscriber
		EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 1);
		EXPECT_EQ(port.barrier.get_num_subscribers(), 1);
	}

	port_collection.expect_port_0_ready_exception = true;
	try
	{
		registry.add_port_activity_subscription(
			"port_0",
			Pipe::worker_fwk::port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Something went wrong"}); }

	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 1);
	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{1}
	);
	auto const& output_ports = registry.get_msg_file_output_ports();
	auto const& port = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(std::size(port.subscriptions), 1);
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 1);
	EXPECT_EQ(port.barrier.get_num_subscribers(), 1);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_add_subcriber)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription(
		"port_0",
		Pipe::worker_fwk::port_activity_subscriber_ref{}
	);

	EXPECT_EQ(
		registry.get_current_subscription_id(),
		Pipe::worker_sync::port_activity_subscription_id{1}
	);
	auto const& output_ports = registry.get_msg_file_output_ports();
	auto const& port = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(std::size(port.subscriptions), 1);
	// This is zero, since callback triggered immediately
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port.barrier.get_num_subscribers(), 1);

	EXPECT_EQ(std::size(registry.get_port_acivity_subscriptions()), 1);
	auto const& subscriptions = registry.get_port_acivity_subscriptions();
	auto const& subscription = subscriptions.at(id);
	EXPECT_EQ(subscription.port, &port);
	EXPECT_EQ(subscription.status, Pipe::worker_fwk::msg_file_input_port_status::ready);
	EXPECT_EQ(subscription.subscriber, Pipe::worker_fwk::port_activity_subscriber_ref{});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready_client_subscription_does_not_exist)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	try
	{
		registry.notify_client_ready(
			Pipe::worker_sync::port_activity_subscription_id{},
			Pipe::worker_fwk::port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready_client_different_owner)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber{};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});

	try
	{
		registry.notify_client_ready(id, Pipe::worker_fwk::port_activity_subscriber_ref{});
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"});
	}
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready_client_client_already_ready)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber{};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});

	try
	{
		// NOTE: port starts in state ready
		registry.notify_client_ready(id, Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});
		REQUIRE_EQ(true, false);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Port activity subscriber is already ready"});
	}
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_client_ready_success)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber{};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});
	subscriber.expected_id = id;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});
	port_collection.expect_port_0_ready = true;
	registry.notify_client_ready(id, Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_notify_data_ready)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber_0{};
	port_collection.expect_port_0_ready = true;
	auto const id_0 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});

	my_activity_subscriber subscriber_1{};
	auto const id_1 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});

	port_collection.expect_port_1_ready = true;
	auto const id_2 = registry.add_port_activity_subscription("port_1", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});

	auto const id_3 = registry.add_port_activity_subscription("port_1", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});

	subscriber_0.expected_id = id_0;
	subscriber_1.expected_id = id_1;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});
	auto const& ports = registry.get_msg_file_output_ports();
	auto const& port_0 = ports.at(Pipe::worker_fwk::port_id{0});
	auto const& port_1 = ports.at(Pipe::worker_fwk::port_id{1});
	auto const& activity_subscriptions = registry.get_port_acivity_subscriptions();
	for(auto const& item : port_0.subscriptions)
	{ EXPECT_EQ(activity_subscriptions.at(item.id).status, Pipe::worker_fwk::msg_file_input_port_status::busy); }
	for(auto const& item : port_1.subscriptions)
	{ EXPECT_EQ(activity_subscriptions.at(item.id).status, Pipe::worker_fwk::msg_file_input_port_status::ready); }

	subscriber_0.expected_id = id_2;
	subscriber_1.expected_id = id_3;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{1});
	for(auto const& item : port_0.subscriptions)
	{ EXPECT_EQ(activity_subscriptions.at(item.id).status, Pipe::worker_fwk::msg_file_input_port_status::busy); }
	for(auto const& item : port_1.subscriptions)
	{ EXPECT_EQ(activity_subscriptions.at(item.id).status, Pipe::worker_fwk::msg_file_input_port_status::busy); }
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_port_activity_subscription_subscription_does_not_exist)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
	try
	{
		registry.remove_port_activity_subscription(
			Pipe::worker_sync::port_activity_subscription_id{},
			Pipe::worker_fwk::port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"}); }
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_port_activity_subscription_different_owner)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber{};
	port_collection.expect_port_0_ready = true;
	auto const id = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber});
	try
	{
		registry.remove_port_activity_subscription(
			id,
			Pipe::worker_fwk::port_activity_subscriber_ref{}
		);
		REQUIRE_EQ(false, true);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Invalid subscription id"}); }

	auto const& activity_subscriptions = registry.get_port_acivity_subscriptions();
	EXPECT_EQ(activity_subscriptions.at(id).port->barrier.get_num_subscribers(), 1);
	EXPECT_EQ(std::size(activity_subscriptions.at(id).port->subscriptions), 1);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_remove_port_activity_subscription)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber_0{};
	port_collection.expect_port_0_ready = true;
	auto const id_0 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});
	subscriber_0.expected_id = id_0;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	my_activity_subscriber subscriber_1{};
	auto const id_1 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});

	registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});
	auto const& ports = registry.get_msg_file_output_ports();
	auto const& port = ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(port.barrier.get_num_subscribers(), 3);
	// Only one is ready, since the first one has already triggered the port ready event
	// NOTE: since no data ready event has been sent, both clients are considered ready
	EXPECT_EQ(port.barrier.get_num_ready_subscribers(), 2);

	// This should not trigger any port ready event since the number of ready client does not change
	registry.remove_port_activity_subscription(
		id_1,
		Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1}
	);

	auto const& activity_subscriptions = registry.get_port_acivity_subscriptions();
	EXPECT_EQ(activity_subscriptions.at(id_0).status, Pipe::worker_fwk::msg_file_input_port_status::busy);

	port_collection.expect_port_0_ready = true;
	registry.remove_port_activity_subscription(
		id_0,
		Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0}
	);
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_send_client_ready_after_adding_an_additional_subscriber)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber_0{};
	port_collection.expect_port_0_ready = true;
	auto const id_0 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});
	subscriber_0.expected_id = id_0;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	my_activity_subscriber subscriber_1{};
	registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});

	port_collection.expect_port_0_ready = true;
	registry.notify_client_ready(id_0, Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_send_client_ready_after_adding_an_additional_subscriber_wake_up_after_second_add)
{
	Pipe::utils::at_scope_exit{
		[saved_offset = malloc_offset](){
			malloc_offset = saved_offset;
		}
	};

	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	my_activity_subscriber subscriber_0{};
	port_collection.expect_port_0_ready = true;
	auto const id_0 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});

	my_activity_subscriber subscriber_1{};
	auto const id_1 = registry.add_port_activity_subscription("port_0", Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_1});
	subscriber_0.expected_id = id_0;
	subscriber_1.expected_id = id_1;
	registry.notify_data_ready(Pipe::worker_fwk::port_id{0});

	registry.notify_client_ready(id_0, Pipe::worker_fwk::port_activity_subscriber_ref{subscriber_0});
}


#endif