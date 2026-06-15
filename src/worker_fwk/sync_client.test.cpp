//@	{"target":{"name":"sync_client.test"}}

#include "./sync_client.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/utils/utils.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct input_port_activity_subscriber
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

		~input_port_activity_subscriber()
		{
			EXPECT_EQ(expected_conn_lost_ptr.has_value(), false);
			EXPECT_EQ(expected_data_ready_id.has_value(), false);
			EXPECT_EQ(expected_subscription_transaction.has_value(), false);
			EXPECT_EQ(expected_subscription_id.has_value(), false);
			EXPECT_EQ(expected_unsubscription_transaction.has_value(), false);
		}
	};

	using sync_client = Pipe::worker_fwk::sync_client<std::reference_wrapper<input_port_activity_subscriber>>;

	struct my_event_handler_registry:Pipe::os_services::fd::activity_event_handler_store
	{
		void remove(Pipe::os_services::fd::event_handler_id id) noexcept override
		{
			EXPECT_EQ(expected_remove_id.has_value(), true);
			EXPECT_EQ(id, expected_remove_id);
			expected_remove_id.reset();
		}

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref handle,
			Pipe::os_services::fd::activity_status new_status
		) override
		{
			REQUIRE_EQ(expected_update_listening_status_call.has_value(), true);
			EXPECT_EQ(handle.get(), expected_update_listening_status_call->handle.get());
			EXPECT_EQ(new_status, expected_update_listening_status_call->new_status);
			expected_update_listening_status_call.reset();
		}

		std::pair<void*, Pipe::os_services::fd::event_handler_id> do_add(
			event_handler_info const& event_handler,
			Pipe::os_services::fd::file_descriptor fd,
			Pipe::os_services::fd::activity_status activity_status
		) override
		{
			REQUIRE_EQ(expected_do_add_call.has_value(), true);
			expected_do_add_call->registred_fd = std::move(fd);
			EXPECT_EQ(expected_do_add_call->initial_listening_status, activity_status);

			saved_event_handler.reset();

			saved_event_handler_buffer = std::make_unique<std::byte[]>(event_handler.object_size);
			event_handler.construct_event_handler_at(
				Pipe::os_services::fd::activity_event_handler_store::dest_object_location{
					.address = saved_event_handler_buffer.get()
				},
				event_handler.object_address
			);

			saved_event_handler = std::unique_ptr<void, void(*)(void*)>{
				saved_event_handler_buffer.get(),
				event_handler.destroy_event_handler_at
			};

			return std::pair{saved_event_handler.get(), expected_do_add_call->retval};
		}

		std::optional<Pipe::os_services::fd::event_handler_id> expected_remove_id;
		std::unique_ptr<std::byte[]> saved_event_handler_buffer;
		std::unique_ptr<void, void(*)(void*)> saved_event_handler{nullptr, nullptr};

		struct update_listening_status_call
		{
			Pipe::os_services::fd::saved_event_handler_ref handle;
			Pipe::os_services::fd::activity_status new_status;
		};
		std::optional<update_listening_status_call> expected_update_listening_status_call;

		struct do_add_call
		{
			Pipe::os_services::fd::activity_status initial_listening_status;
			Pipe::os_services::fd::file_descriptor registred_fd;
			Pipe::os_services::fd::event_handler_id retval;
		};
		std::optional<do_add_call> expected_do_add_call;
	};

	auto make_sockets()
	{
		Pipe::os_services::ipc::socket_pair<SOCK_STREAM> sockets;
		auto const flags = ::fcntl(sockets.socket_b().native_handle(), F_GETFL);
		REQUIRE_NE(flags, -1);
		auto const res = ::fcntl(sockets.socket_b().native_handle(), F_SETFL, O_NONBLOCK);
		REQUIRE_NE(res, -1);
		return sockets;
	}

	template<class MsgType, size_t ExpectedByteCount>
	MsgType receive_message(Pipe::os_services::io::input_file_descriptor_ref fd)
	{
		std::array<std::byte, ExpectedByteCount> buffer;
		auto const read_result = read_full(fd, buffer);
		REQUIRE_EQ(read_result.operation_would_have_blocked(), false);
		REQUIRE_EQ(read_result.bytes_transferred(), ExpectedByteCount);
		Pipe::worker_sync::decoder<MsgType> decoder{};
		auto const res = decoder.decode(buffer);
		EXPECT_EQ(res, ExpectedByteCount);
		EXPECT_EQ(decoder.completed(), true);
		return decoder.get_value();
	}

};

TESTCASE(Pipe_worker_fwk_sync_client_construct)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_without_tx_id)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	Pipe::worker_sync::exception_controller ec;
	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The error message"});
	}
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_no_ongoing_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	Pipe::worker_sync::exception_controller ec;
	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{4324},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"});
	}
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}


TESTCASE(Pipe_worker_fwk_sync_client_handle_error_response_existing_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"My port",
		input_port_activity_subscriber::subscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::error_response{
				.message = "The error message"
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"The error message"});
	}
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_data_ready_event)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
	subscriber.expected_data_ready_id = Pipe::worker_sync::port_activity_subscription_id{3};
	client.handle_message(
		Pipe::worker_sync::data_ready_event{
			.id = Pipe::worker_sync::port_activity_subscription_id{3}
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_no_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_subscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_wrong_transaction_type)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.unsubscribe_from_port(
		Pipe::worker_sync::port_activity_subscription_id{435},
		input_port_activity_subscriber::unsubscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_subscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Unexpected transaction type"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"The port",
		input_port_activity_subscriber::subscription_transaction{1}
	);

	subscriber.expected_subscription_transaction = input_port_activity_subscriber::subscription_transaction{1};
	subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{435};
	client.handle_response(
		Pipe::worker_sync::port_activity_subscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{0},
		ec
	);

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response_no_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_unsubscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response_wrong_transaction_type)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"The port",
		input_port_activity_subscriber::subscription_transaction{}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_unsubscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Unexpected transaction type"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_unsubscription_response)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.unsubscribe_from_port(
		Pipe::worker_sync::port_activity_subscription_id{435},
		input_port_activity_subscriber::unsubscription_transaction{1}
	);

	subscriber.expected_unsubscription_transaction = input_port_activity_subscriber::unsubscription_transaction{1};
	client.handle_response(
		Pipe::worker_sync::port_activity_unsubscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{0},
		ec
	);

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);
}

TESTCASE(Pipe_worker_fwk_sync_client_notify_client_ready)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	auto const sockets = make_sockets();
	my_event_handler_registry eh_registry;
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	client.handle_event(
		sync_client::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	client.notify_client_ready(Pipe::worker_sync::port_activity_subscription_id{44});

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	std::ignore = client.send_pending_messages();

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.tx_id.is_valid(), false);
	EXPECT_EQ(header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::client_ready_event,
				Pipe::worker_sync::client_to_server_message
			>
		)
	)

	auto const body = receive_message<Pipe::worker_sync::client_ready_event, 8>(sockets.socket_b());
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{44});

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_subscribe_to_port)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	auto const sockets = make_sockets();
	my_event_handler_registry eh_registry;
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	client.handle_event(
		sync_client::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	client.subscribe_to_port(
		"some_port",
		input_port_activity_subscriber::subscription_transaction{23}
	);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	std::ignore = client.send_pending_messages();

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{0});
	EXPECT_EQ(header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::port_activity_subscription_request,
				Pipe::worker_sync::client_to_server_message
			>
		)
	)

	auto const body = receive_message<
		Pipe::worker_sync::port_activity_subscription_request,
		17
	>(sockets.socket_b());
	EXPECT_EQ(body.server_portname, "some_port");

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_unsubscribe_from_port)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	auto const sockets = make_sockets();
	my_event_handler_registry eh_registry;
	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	client.handle_event(
		sync_client::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	client.unsubscribe_from_port(
		Pipe::worker_sync::port_activity_subscription_id{6},
		input_port_activity_subscriber::unsubscription_transaction{23}
	);

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	std::ignore = client.send_pending_messages();

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{0});
	EXPECT_EQ(header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::port_activity_unsubscription,
				Pipe::worker_sync::client_to_server_message
			>
		)
	)

	auto const body = receive_message<
		Pipe::worker_sync::port_activity_unsubscription,
		8
	>(sockets.socket_b());
	EXPECT_EQ(body.id, Pipe::worker_sync::port_activity_subscription_id{6});

	subscriber.expected_conn_lost_ptr = &client;
}

TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_nonexisting_transaction)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"The port",
		input_port_activity_subscriber::subscription_transaction{1}
	);

	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_subscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{1},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no matching ongoing transaction"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}


TESTCASE(Pipe_worker_fwk_sync_client_handle_subscription_response_transactions_out_of_order)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_sync::exception_controller ec;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};

	client.subscribe_to_port(
		"port_a",
		input_port_activity_subscriber::subscription_transaction{1}
	);
	client.subscribe_to_port(
		"port_b",
		input_port_activity_subscriber::subscription_transaction{2}
	);

	subscriber.expected_subscription_transaction = input_port_activity_subscriber::subscription_transaction{2};
	subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{435};
	ec.disable_exception_rethrow();
	client.handle_response(
		Pipe::worker_sync::port_activity_subscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{435}
		},
		Pipe::worker_sync::transaction_id{1},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);

	subscriber.expected_subscription_transaction = input_port_activity_subscriber::subscription_transaction{1};
	subscriber.expected_subscription_id = Pipe::worker_sync::port_activity_subscription_id{436};
	ec.disable_exception_rethrow();
	client.handle_response(
		Pipe::worker_sync::port_activity_subscription_response{
			.id = Pipe::worker_sync::port_activity_subscription_id{436}
		},
		Pipe::worker_sync::transaction_id{0},
		ec
	);
	EXPECT_EQ(ec.exceptions_should_be_rethrown(), true);

	ec.disable_exception_rethrow();
	try
	{
		client.handle_response(
			Pipe::worker_sync::port_activity_unsubscription_response{
				.id = Pipe::worker_sync::port_activity_subscription_id{435}
			},
			Pipe::worker_sync::transaction_id{0},
			ec
		);
		EXPECT_EQ(true, false);
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Client has no outstanding transactions"}); }

	EXPECT_EQ(ec.exceptions_should_be_rethrown(), false);
}

TESTCASE(Pipe_worker_fwk_make_sync_client)
{
	input_port_activity_subscriber subscriber;
	my_event_handler_registry eh_registry;

	auto const socket_name = Pipe::utils::random_printable_ascii_string(
		Pipe::os_services::ipc::abstract_sunpath_maxlength
	);
	auto const server_socket = Pipe::os_services::ipc::make_server_socket<SOCK_STREAM>(
		Pipe::os_services::ipc::make_abstract_sockaddr_un(socket_name),
		1024
	);
	eh_registry.expected_do_add_call = my_event_handler_registry::do_add_call{
		Pipe::os_services::fd::activity_status::read,
		Pipe::os_services::fd::file_descriptor{},
		Pipe::os_services::fd::event_handler_id{324}
	};
	auto const result = Pipe::worker_fwk::make_sync_client(eh_registry, std::ref(subscriber), socket_name);
	EXPECT_EQ(result.second, Pipe::os_services::fd::event_handler_id{324});

	eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read_or_write
	};
	result.first.get().handle_event(
		sync_client::sync_fd_activity_event_handler_registred_event{
			.fd = Pipe::os_services::fd::tagged_file_descriptor_ref<
				Pipe::os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>
			>(eh_registry.expected_do_add_call->registred_fd.get().native_handle()),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	subscriber.expected_conn_lost_ptr = &result.first.get();

	auto const server_fd = accept(server_socket.get());
	{
		auto const fd = server_fd.get().native_handle();
		auto const flags = ::fcntl(fd, F_GETFL);
		assert(flags != -1);
		::fcntl(fd, F_SETFL, O_NONBLOCK|flags);
	}

	result.first.get().subscribe_to_port(
		"port_name",
		input_port_activity_subscriber::subscription_transaction{}
	);
	EXPECT_EQ(result.first.get().is_connected(), true);
		eh_registry.expected_update_listening_status_call = my_event_handler_registry::update_listening_status_call{
		.handle = Pipe::os_services::fd::saved_event_handler_ref{},
		.new_status = Pipe::os_services::fd::activity_status::read
	};
	std::ignore = result.first.get().send_pending_messages();
	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(server_fd.get());
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{0});
	EXPECT_EQ(header.msg_id,
		(
			Pipe::utils::variant_index_v<
				Pipe::worker_sync::port_activity_subscription_request,
				Pipe::worker_sync::client_to_server_message
			>
		)
	);

	auto const body = receive_message<Pipe::worker_sync::port_activity_subscription_request, 17>(
		server_fd.get()
	);
	EXPECT_EQ(body.server_portname, "port_name");
}