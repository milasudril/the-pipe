//@	{"target":{"name":"sync_link.test"}}

#include "./sync_client.hpp"
#include "./sync_server.hpp"
#include "./msg_file_subscription_registry.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/eventfd.hpp"
#include "src/os_services/io_multiplexer/epoll_instance.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/worker_fwk/port_activity_subscriber.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <thread>
#include <future>

#include <testfwk/testfwk.hpp>

namespace
{
	template <class ArgType>
	class task_queue
	{
		public:
			void push(std::move_only_function<void(ArgType)>&& value)
			{
				std::lock_guard<std::mutex> lock{m_mtx};
				m_queue.push(std::move(value));
				m_cv.notify_all();
				if(m_registration.is_valid())
				{ flush_events(); }
			}

			using fd_activity_event_hanadler_registred_event =
				Pipe::os_services::fd::activity_event_handler_registered_event<
					void,
					Pipe::os_services::ipc::eventfd_tag
				>;

			using fd_activity_event = Pipe::os_services::fd::activity_event<
				void,
				Pipe::os_services::ipc::eventfd_tag
			>;

			void handle_event(fd_activity_event_hanadler_registred_event const& registration)
			{
				m_registration = registration;
				if(!m_queue.empty())
				{ flush_events(); }
			}

			void handle_event(fd_activity_event event)
			{
				if(can_read(event.status))
				{
					std::array<std::byte, 8> buffer;
					auto res = Pipe::os_services::io::read_full(m_registration.fd, buffer);
					EXPECT_EQ(res.bytes_transferred(), 8);
					drain_queue();
				}
			}

			void drain_queue()
			{
				while(true)
				{
					std::unique_lock lock{m_mtx};
					if(m_queue.empty())
					{
						m_cv.notify_all();
						return;
					}

					auto next = std::move(m_queue.front());
					m_queue.pop();
					lock.unlock();
					next(m_ctxt);
				}
			}

			void wait_for_tasks()
			{
				std::unique_lock lock{m_mtx};
				m_cv.wait(lock, [this](){
					return !m_queue.empty();
				});
			}

			void set_context(ArgType ctxt)
			{ m_ctxt = ctxt; }

			void synchronize()
			{
				wait_for_tasks();
				drain_queue();
			}

			void wait_for_empty()
			{
				std::unique_lock lock{m_mtx};
				m_cv.wait(lock, [this](){
					return m_queue.empty();
				});
			}

			void notify()
			{ push([](ArgType){}); }

		private:
			std::queue<std::move_only_function<void(ArgType)>> m_queue;
			std::mutex m_mtx;
			fd_activity_event_hanadler_registred_event m_registration;
			ArgType m_ctxt{};
			std::condition_variable m_cv;

			void flush_events()
			{
				auto const buffer = std::bit_cast<std::array<std::byte, 8>>(uint64_t{1});
				auto const res = Pipe::os_services::io::write_full(m_registration.fd, buffer);
				EXPECT_EQ(res.bytes_transferred(), 8);
			}
	};


	struct testcase_context
	{
		std::string server_socket_name;
	};

	struct test_port_collection
	{
		task_queue<testcase_context*>* testcase_tasks;

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
			Pipe::utils::at_scope_exit _{
				[testcase_tasks = testcase_tasks](){
					testcase_tasks->notify();
				}
			};
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
		void port_0_ready()
		{
			EXPECT_EQ(expect_port_0_ready, true);
			expect_port_0_ready = false;
			testcase_tasks->notify();
		}

		bool expect_port_1_ready = false;
		void port_1_ready()
		{
			EXPECT_EQ(expect_port_1_ready, true);
			expect_port_1_ready = false;
			testcase_tasks->notify();
		}

		bool expect_port_2_ready = false;
		void port_2_ready()
		{
			EXPECT_EQ(expect_port_2_ready, true);
			expect_port_2_ready = false;
			testcase_tasks->notify();
		}

		bool expect_port_3_ready = false;
		void port_3_ready()
		{
			EXPECT_EQ(expect_port_3_ready, true);
			expect_port_3_ready = false;
			testcase_tasks->notify();
		}

		bool expect_port_4_ready = false;
		void port_4_ready()
		{
			EXPECT_EQ(expect_port_4_ready, true);
			expect_port_4_ready = false;
			testcase_tasks->notify();
		}

		~test_port_collection()
		{
			EXPECT_EQ(expect_port_0_ready, false);
			EXPECT_EQ(expect_port_1_ready, false);
			EXPECT_EQ(expect_port_2_ready, false);
			EXPECT_EQ(expect_port_3_ready, false);
			EXPECT_EQ(expect_port_4_ready, false);
			testcase_tasks->notify();
		}
	};

	struct test_input_port_activity_subscriber
	{
		task_queue<testcase_context*>* testcase_tasks;

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
			testcase_tasks->notify();
		}

		std::optional<Pipe::worker_sync::port_activity_subscription_id> expected_data_ready_id;
		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id id)
		{
			EXPECT_EQ(expected_data_ready_id.has_value(), true);
			EXPECT_EQ(expected_data_ready_id, id);
			expected_data_ready_id.reset();
			testcase_tasks->notify();
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
			testcase_tasks->notify();
		}

		std::optional<unsubscription_transaction> expected_unsubscription_transaction;
		void unsubscription_completed(unsubscription_transaction transaction)
		{
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), true);
			EXPECT_EQ(expected_unsubscription_transaction, transaction);
			expected_unsubscription_transaction.reset();
			testcase_tasks->notify();
		}

		~test_input_port_activity_subscriber()
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), false);
			EXPECT_EQ(expected_data_ready_id.has_value(), false);
			EXPECT_EQ(expected_subscription_transaction.has_value(), false);
			EXPECT_EQ(expected_subscription_id.has_value(), false);
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), false);
			testcase_tasks->notify();
		}
	};

	struct server_context
	{
		test_port_collection port_collection;
		Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
		bool should_exit{false};
	};

	void run_server(
		task_queue<server_context*>& server_tasks,
		task_queue<testcase_context*>& testcase_tasks
	)
	{
		server_context ctxt{};
		ctxt.port_collection.testcase_tasks = &testcase_tasks;
		server_tasks.set_context(&ctxt);
		Pipe::os_services::io_multiplexer::epoll_instance epoll;

		std::ignore = epoll.add<void>(
			std::ref(server_tasks),
			Pipe::os_services::ipc::make_eventfd(),
			Pipe::os_services::fd::activity_status::read
		);

		auto const server = Pipe::worker_fwk::make_sync_server(epoll, std::ref(ctxt.registry));

		testcase_tasks.push([socket_name = server.socket_name](testcase_context* ctxt) mutable {
			REQUIRE_NE(ctxt, nullptr);
			ctxt->server_socket_name = std::move(socket_name);
		});

		while(!ctxt.should_exit)
		{ epoll.wait_for_and_distpatch_events(); }
	}

	struct client_context
	{
		test_input_port_activity_subscriber activity_subscriber;
		Pipe::worker_fwk::sync_client<std::reference_wrapper<test_input_port_activity_subscriber>>* client;
		std::optional<std::string> expected_error_message;
		bool should_exit{false};
	};

	void run_client(
		task_queue<client_context*>& client_tasks,
		task_queue<testcase_context*>& testcase_tasks,
		std::string server_socket_name
	)
	{
		client_context ctxt{};
		ctxt.activity_subscriber.testcase_tasks = &testcase_tasks;
		client_tasks.set_context(&ctxt);
		Pipe::os_services::io_multiplexer::epoll_instance epoll;

		std::ignore = epoll.add<void>(
			std::ref(client_tasks),
			Pipe::os_services::ipc::make_eventfd(),
			Pipe::os_services::fd::activity_status::read
		);

		auto const client = Pipe::worker_fwk::make_sync_client(
			epoll,
			std::ref(ctxt.activity_subscriber),
			server_socket_name
		);
		ctxt.client = &client.first.get();
		testcase_tasks.notify();

		while(!ctxt.should_exit)
		{
			if(ctxt.expected_error_message.has_value())
			{
				try
				{ epoll.wait_for_and_distpatch_events(); }
				catch(std::exception const& err)
				{
					EXPECT_EQ(err.what(), ctxt.expected_error_message);
					ctxt.expected_error_message.reset();
					testcase_tasks.notify();
				}
			}
			else
			{ epoll.wait_for_and_distpatch_events(); }
		}
		EXPECT_EQ(ctxt.expected_error_message.has_value(), false);
	}
}

TESTCASE(Pipe_worker_fwk_sync_link_subscribe_and_send_notifications)
{
	testcase_context ctxt;
	task_queue<server_context*> server_tasks;
	task_queue<testcase_context*> server_barry;
	task_queue<client_context*> client_tasks;
	task_queue<testcase_context*> client_barry;
	server_barry.set_context(&ctxt);
	client_barry.set_context(&ctxt);

	std::jthread server_thread{run_server, std::ref(server_tasks), std::ref(server_barry)};
	server_barry.synchronize();

	std::jthread client_thread{
		run_client,
		std::ref(client_tasks),
		std::ref(client_barry),
		ctxt.server_socket_name
	};
	client_barry.synchronize();

	// Subscribe to port_0
	{
		server_tasks.push([](server_context* ctxt){
			ctxt->port_collection.expect_port_0_ready = true;
		});
		server_tasks.wait_for_empty();

		client_tasks.push([](client_context* ctxt){
			test_input_port_activity_subscriber::subscription_transaction transaction{1};
			ctxt->activity_subscriber.expected_subscription_transaction = transaction;
			ctxt->activity_subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{0};
			ctxt->client->subscribe_to_port(
				"port_0",
				std::move(transaction)
			);
		});
		client_barry.synchronize();
		server_barry.synchronize();
	}

	// Notify client that there is data
	{
		client_tasks.push([](client_context* ctxt) {
			ctxt->activity_subscriber.expected_data_ready_id = Pipe::worker_sync::port_activity_subscription_id{0};
		});
		client_tasks.wait_for_empty();

		server_tasks.push([](server_context* ctxt) {
			ctxt->registry.notify_data_ready(Pipe::worker_fwk::port_id{0});
		});
		client_barry.synchronize();
	}

	// Notify server that data has been processed
	{
		server_tasks.push([](server_context* ctxt){
			ctxt->port_collection.expect_port_0_ready = true;
		});
		server_tasks.wait_for_empty();

		client_tasks.push([](client_context* ctxt) {
			ctxt->client->notify_client_ready(Pipe::worker_sync::port_activity_subscription_id{0});
		});
		server_barry.synchronize();
	}

	client_tasks.push([](client_context* ctxt){
		ctxt->activity_subscriber.expected_conn_lost_ptr = ctxt->client;
		ctxt->should_exit = true;
	});
	client_tasks.wait_for_empty();

	server_tasks.push([](server_context* ctxt){
		ctxt->should_exit = true;
	});
}

TESTCASE(Pipe_worker_fwk_sync_link_notify_client_ready_on_nonexisting_subscription)
{
	testcase_context ctxt;
	task_queue<server_context*> server_tasks;
	task_queue<testcase_context*> server_barry;
	task_queue<client_context*> client_tasks;
	task_queue<testcase_context*> client_barry;
	server_barry.set_context(&ctxt);
	client_barry.set_context(&ctxt);

	std::jthread server_thread{run_server, std::ref(server_tasks), std::ref(server_barry)};
	server_barry.synchronize();

	std::jthread client_thread{
		run_client,
		std::ref(client_tasks),
		std::ref(client_barry),
		ctxt.server_socket_name
	};
	client_barry.synchronize();


	client_tasks.push([](client_context* ctxt) {
		// Causes exception at client side because client misbehaves
		ctxt->expected_error_message = "Invalid subscription id";
		ctxt->client->notify_client_ready(Pipe::worker_sync::port_activity_subscription_id{0});
	});
	client_barry.synchronize();

	client_tasks.push([](client_context* ctxt){
		ctxt->activity_subscriber.expected_conn_lost_ptr = ctxt->client;
		ctxt->should_exit = true;
	});
	client_tasks.wait_for_empty();

	server_tasks.push([](server_context* ctxt){
		ctxt->should_exit = true;
	});
}

TESTCASE(Pipe_worker_fwk_sync_link_unsubscribe_from_nonexisting_subscription)
{
	testcase_context ctxt;
	task_queue<server_context*> server_tasks;
	task_queue<testcase_context*> server_barry;
	task_queue<client_context*> client_tasks;
	task_queue<testcase_context*> client_barry;
	server_barry.set_context(&ctxt);
	client_barry.set_context(&ctxt);

	std::jthread server_thread{run_server, std::ref(server_tasks), std::ref(server_barry)};
	server_barry.synchronize();

	std::jthread client_thread{
		run_client,
		std::ref(client_tasks),
		std::ref(client_barry),
		ctxt.server_socket_name
	};
	client_barry.synchronize();


	client_tasks.push([](client_context* ctxt) {
		// Causes exception at client side because client misbehaves
		ctxt->expected_error_message = "Invalid subscription id";
		ctxt->client->unsubscribe_from_port(
			Pipe::worker_sync::port_activity_subscription_id{0},
			test_input_port_activity_subscriber::unsubscription_transaction{23}
		);
	});
	client_barry.synchronize();

	client_tasks.push([](client_context* ctxt){
		ctxt->activity_subscriber.expected_conn_lost_ptr = ctxt->client;
		ctxt->should_exit = true;
	});
	client_tasks.wait_for_empty();

	server_tasks.push([](server_context* ctxt){
		ctxt->should_exit = true;
	});
}