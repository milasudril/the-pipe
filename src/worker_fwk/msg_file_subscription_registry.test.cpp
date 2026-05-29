//@	{"target":{"name":"msg_file_subscription_registry.test"}}

#include "./msg_file_subscription_registry.hpp"
#include "src/worker_sync/worker_sync.hpp"

#include <testfwk/testfwk.hpp>

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

		void port_0_ready()
		{}

		void port_1_ready()
		{}

		void port_2_ready()
		{}

		void port_3_ready()
		{}

		void port_4_ready()
		{}

		Pipe::worker_fwk::port_id get_port_id(std::string const&)
		{
			return Pipe::worker_fwk::port_id{};
		}
	};
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_construct_with_port_collection)
{
	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};

	auto const& output_ports = registry.get_msg_file_output_ports();
	EXPECT_EQ(std::size(output_ports), 5);

	auto const& port_0 = output_ports.at(Pipe::worker_fwk::port_id{0});
	EXPECT_EQ(std::size(port_0.subscriptions), 0);
	EXPECT_EQ(port_0.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_0.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_0.barrier.is_bond_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_0_ready>(port_collection)
		),
		true
	);

	auto const& port_1 = output_ports.at(Pipe::worker_fwk::port_id{1});
	EXPECT_EQ(std::size(port_1.subscriptions), 0);
	EXPECT_EQ(port_1.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_1.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_1.barrier.is_bond_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_1_ready>(port_collection)
		),
		true
	);

	auto const& port_2 = output_ports.at(Pipe::worker_fwk::port_id{2});
	EXPECT_EQ(std::size(port_2.subscriptions), 0);
	EXPECT_EQ(port_2.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_2.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_2.barrier.is_bond_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_2_ready>(port_collection)
		),
		true
	);

	auto const& port_3 = output_ports.at(Pipe::worker_fwk::port_id{3});
	EXPECT_EQ(std::size(port_3.subscriptions), 0);
	EXPECT_EQ(port_3.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_3.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_3.barrier.is_bond_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_3_ready>(port_collection)
		),
		true
	);

	auto const& port_4 = output_ports.at(Pipe::worker_fwk::port_id{4});
	EXPECT_EQ(std::size(port_4.subscriptions), 0);
	EXPECT_EQ(port_4.barrier.get_num_ready_subscribers(), 0);
	EXPECT_EQ(port_4.barrier.get_num_subscribers(), 0);
	EXPECT_EQ(
		port_4.barrier.is_bond_to(
			Pipe::utils::bind_member_function<&my_port_collection::port_4_ready>(port_collection)
		),
		true
	);

	auto const& activity_subscriptions = registry.get_port_acivity_subscriptions();
	EXPECT_EQ(std::size(activity_subscriptions), 0);

	auto const current_subscription_id = registry.get_current_subscription_id();
	EXPECT_EQ(current_subscription_id, Pipe::worker_sync::port_activity_subscription_id{0});
}