//@	{"target":{"name":"sync_link.test"}}

#include "./sync_client.hpp"
#include "./sync_server.hpp"
#include "./msg_file_subscription_registry.hpp"

#include "src/os_services/io_multiplexer/epoll_instance.hpp"

#include <thread>
#include <future>

#include <testfwk/testfwk.hpp>

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

	struct test_input_port_activity_subscriber
	{
		struct subscription_transaction
		{
			int value;
			constexpr auto operator<=>(subscription_transaction const&) const = default;
		};

		struct unsubscription_transaction
		{
			int value;
			constexpr auto operator<=>(unsubscription_transaction const&) const = default;
		};

		std::optional<void const*> expected_conn_lost_ptr;
		void sync_client_lost_connection_to_server(void const* ptr)
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), true);
			EXPECT_EQ(expected_conn_lost_ptr, ptr);
			expected_conn_lost_ptr.reset();
		}

		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_data_ready_id;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(expected_data_ready_id.has_value(), true);
			EXPECT_EQ(expected_data_ready_id, id);
			expected_data_ready_id.reset();
		}

		std::optional<subscription_transaction> expected_subscription_transaction;
		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_subscription_id;
		void subscription_completed(
			subscription_transaction transaction,
			Pipe::worker_sync::port_activity_subscription_id subscription_id
		)
		{
			EXPECT_EQ(expected_subscription_transaction.has_value(), true);
			EXPECT_EQ(expected_subscription_transaction, transaction);
			expected_subscription_transaction.reset();
			EXPECT_EQ(expected_subscription_id.has_value(), true);
			EXPECT_EQ(subscription_id, expected_subscription_id);
			expected_subscription_id.reset();
		}

		std::optional<unsubscription_transaction> expected_unsubscription_transaction;
		void unsubscription_completed(unsubscription_transaction transaction)
		{
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), true);
			EXPECT_EQ(expected_unsubscription_transaction, transaction);
			expected_unsubscription_transaction.reset();
		}

		~test_input_port_activity_subscriber()
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), false);
			EXPECT_EQ(expected_data_ready_id.has_value(), false);
			EXPECT_EQ(expected_subscription_transaction.has_value(), false);
			EXPECT_EQ(expected_subscription_id.has_value(), false);
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), false);
		}
	};
};

TESTCASE(Pipe_worker_fwk_sync_link_subscribe_and_send_notifications)
{
	std::promise<std::string> server_name_promise;
	test_port_collection port_collection;
	std::jthread server_thread{
		[&server_name_promise, &port_collection](){
			Pipe::os_services::io_multiplexer::epoll_instance epoll;
			auto const server = Pipe::worker_fwk::make_sync_server(
				epoll,
				Pipe::worker_fwk::msg_file_subscription_registry{port_collection}
			);
			server_name_promise.set_value(server.socket_name);
			do
			{
				epoll.wait_for_and_distpatch_events();
			}
			while(epoll.get_num_listeners() > 1);
		}
	};

	auto server_name = server_name_promise.get_future().get();
	test_input_port_activity_subscriber activity_subscriber;
	Pipe::os_services::io_multiplexer::epoll_instance epoll;
	void* conn_lost_ptr;
	{
		auto const sync_client = Pipe::worker_fwk::make_sync_client(epoll, std::ref(activity_subscriber), server_name);

		auto& client = sync_client.first.get();

		EXPECT_EQ(client.is_connected(), true);
		conn_lost_ptr = &client;
	}
	activity_subscriber.expected_conn_lost_ptr = conn_lost_ptr;
}