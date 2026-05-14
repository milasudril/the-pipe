//@	{"target":{"name":"sync_message_channel.test"}}

#include "./sync_message_channel.hpp"
#include "src/os_services/error_handling/system_error.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/utils/utils.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include "testfwk/validation.hpp"

#include <limits>
#include <sys/syscall.h>
#include <testfwk/testfwk.hpp>
#include <signal.h>

namespace
{
	struct request
	{
		int value;
	};

	struct response
	{
		int value;
	};

	struct notification
	{
		int value;
	};

	struct my_error_handler
	{

	};

	struct msg_channel_traits
	{
		using fd_tag = Pipe::os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;
		using incoming_sync_msg_type = std::variant<request, notification>;
		using outgoing_sync_msg_type = std::variant<response, Pipe::worker_sync::error_response>;
		struct client_activity{};
		using sync_fd_activity_event_handler_registred_event =
			Pipe::os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using sync_fd_activity_event = Pipe::os_services::fd::activity_event<client_activity, fd_tag>;
		using error_handler = my_error_handler;
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

	struct event_handler_store:Pipe::os_services::fd::activity_event_handler_store
	{
		std::optional<Pipe::os_services::fd::event_handler_id> id_to_remove;
		std::optional<Pipe::os_services::fd::activity_status> current_listening_status;

		void remove(Pipe::os_services::fd::event_handler_id id) noexcept override
		{
			EXPECT_EQ(id, id_to_remove);
			id_to_remove.reset();
		}

		Pipe::os_services::error_handling::code update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref,
			Pipe::os_services::fd::activity_status status
		) noexcept override
		{
			EXPECT_NE(status, current_listening_status);
			current_listening_status = status;
			return Pipe::os_services::error_handling::code{};
		}

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{
			throw std::runtime_error{"Unexpected function call"};
		}

		~event_handler_store()
		{ EXPECT_EQ(id_to_remove.has_value(), false); }
	};

	struct msg_handler:Pipe::worker_fwk::sync_message_channel<msg_channel_traits>
	{
		using Pipe::worker_fwk::sync_message_channel<msg_channel_traits>::sync_message_channel;
		using Pipe::worker_fwk::sync_message_channel<msg_channel_traits>::handle_message;

		std::optional<int> expected_request_value;
		std::optional<Pipe::worker_sync::transaction_id> expected_transaction_id;
		std::optional<int> expected_notification_value;
		std::optional<std::string> request_exception_string;
		std::optional<std::string> notification_exception_string;

		void handle_request(request req, Pipe::worker_sync::transaction_id id)
		{
			EXPECT_EQ(req.value, expected_request_value);
			expected_request_value.reset();
			EXPECT_EQ(id, expected_transaction_id);
			expected_transaction_id.reset();
			if(request_exception_string.has_value())
			{
				auto str = std::exchange(request_exception_string, std::optional<std::string>{});
				throw std::runtime_error{*str};
			}
		}

		void handle_message(notification notification)
		{
			EXPECT_EQ(notification.value, expected_notification_value);
			expected_notification_value.reset();
			if(notification_exception_string.has_value())
			{
				auto str = std::exchange(notification_exception_string, std::optional<std::string>{});
				throw std::runtime_error{*str};
			}
		}

		~msg_handler()
		{
			EXPECT_EQ(expected_request_value.has_value(), false);
			EXPECT_EQ(expected_transaction_id.has_value(), false);
			EXPECT_EQ(expected_notification_value.has_value(), false);
		}
	};

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

	template<size_t BuffSize, class MsgType>
	void send_message(
		MsgType&& msg,
		Pipe::os_services::io::output_file_descriptor_ref fd
	)
	{
		std::array<std::byte, BuffSize> buffer;
		Pipe::worker_sync::encoder<MsgType> encoder{std::forward<MsgType>(msg)};
		auto const res = encoder.encode(buffer);
		REQUIRE_EQ(res, BuffSize);
		REQUIRE_EQ(encoder.completed(), true);
		auto const write_result = write_full(fd, buffer);
		REQUIRE_EQ(write_result.bytes_transferred(), BuffSize);
		REQUIRE_EQ(write_result.operation_would_have_blocked(), false);
	}

	struct fd_props
	{
		size_t bytes_written{0};
		size_t fake_eagain_above = std::numeric_limits<size_t>::max();
	};

	std::unordered_map<int, fd_props> fd_map;
}

extern "C"
{
	int close(int fd)
	{
		fd_map.erase(fd);
		return static_cast<int>(syscall(SYS_close, fd));
	}

	ssize_t write(int fd, void const* buffer, size_t buff_size)
	{
		// Only intercept non-standard streams
		if(fd <= 2)
		{ return syscall(SYS_write, fd, buffer, buff_size); }


		auto const fd_entry = fd_map.insert(std::pair{fd, fd_props{}});
		auto const max_size = fd_entry.first->second.fake_eagain_above;
		if(fd_entry.first->second.bytes_written >= max_size)
		{
			errno = EAGAIN;
			return - 1;
		}

		auto const res = syscall(SYS_write, fd, buffer, std::min(buff_size, max_size));
		if(res == -1)
		{ return res; }

		fd_entry.first->second.bytes_written += res;
		return res;
	}
}


// Management

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_event_fd_activity_event_handler_registered_event)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	{
		auto const is_blocking = ::fcntl(sockets.socket_a().native_handle(), F_GETFL);
		REQUIRE_NE(is_blocking, -1);
		EXPECT_EQ(is_blocking&O_NONBLOCK, O_NONBLOCK);
	}
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_event_fd_activity_event_handler_registered_event_with_message_queued)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});
	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	{
		auto const is_blocking = ::fcntl(sockets.socket_a().native_handle(), F_GETFL);
		REQUIRE_NE(is_blocking, -1);
		EXPECT_EQ(is_blocking&O_NONBLOCK, O_NONBLOCK);
	}
}

// Reading

TESTCASE(Pipe_worker_fwk_sync_message_channel_read_and_dispatch_requests_no_bytes_ready_operation_would_have_blocked)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::operation_would_have_blocked);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_read_and_dispatch_requests_no_bytes_ready_fd_closed)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	sockets.close_socket_b();
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::remote_endpoint_closed);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_read_and_dispatch_requests_process_request)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<request, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{325}
		},
		sockets.socket_b()
	);
	send_message<4>(request{.value = 43}, sockets.socket_b());
	codec.expected_transaction_id = Pipe::worker_sync::transaction_id{325};
	codec.expected_request_value = 43;
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_read_and_dispatch_requests_process_notification)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<notification, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{325}
		},
		sockets.socket_b()
	);
	send_message<4>(notification{.value = 43}, sockets.socket_b());
	codec.expected_notification_value = 43;
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_read_and_dispatch_requests_short_buffer)
{
	event_handler_store eh_registry;
	msg_handler codec{17};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<notification, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{325}
		},
		sockets.socket_b()
	);
	send_message<4>(notification{.value = 43}, sockets.socket_b());
	auto result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);

	codec.expected_notification_value = 43;
	result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
}

// Sending

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_message_before_registration)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});
	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_message_after_registration)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_no_pending_messages_bytes_to_write_empty)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);
	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status.has_value(), false);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_no_pending_messages_bytes_to_write)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});

	fd_map.insert(std::pair{
		sockets.socket_a().native_handle(),
		fd_props{
			.bytes_written = 0,
			.fake_eagain_above = 0
		}
	});

	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::operation_would_have_blocked);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_no_pending_messages_bytes_to_write_conn_closed)
{
	signal(SIGPIPE, SIG_IGN);
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});

	sockets.close_socket_b();

	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::remote_endpoint_closed);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_write_partial_then_block)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});

	fd_map.insert(std::pair{
		sockets.socket_a().native_handle(),
		fd_props{
			.bytes_written = 0,
			.fake_eagain_above = 17
		}
	});

	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::operation_would_have_blocked);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	codec.send(response{.value = 187}, Pipe::worker_sync::transaction_id{25});
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_write_full)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});

	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.msg_id, (Pipe::utils::variant_index_v<response, msg_channel_traits::outgoing_sync_msg_type>));
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const response_msg = receive_message<response, 4>(sockets.socket_b());
	EXPECT_EQ(response_msg.value, 13);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_send_pending_messages_write_full_small_buffer)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_message_channel<msg_channel_traits> codec{17};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 13}, Pipe::worker_sync::transaction_id{24});

	auto const io_result =codec.send_pending_messages();
	EXPECT_EQ(io_result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(header.msg_id, (Pipe::utils::variant_index_v<response, msg_channel_traits::outgoing_sync_msg_type>));
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const response_msg = receive_message<response, 4>(sockets.socket_b());
	EXPECT_EQ(response_msg.value, 13);
}


// Error handling

TESTCASE(Pipe_worker_fwk_sync_message_channel_receive_unsupport_message_bad_number)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = std::numeric_limits<decltype(Pipe::worker_sync::msg_header::msg_id)>::max(),
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);

	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	auto const send_result = codec.send_pending_messages();
	EXPECT_EQ(send_result, msg_handler::io_status::ok);
	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<Pipe::worker_sync::error_response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const msg = receive_message<Pipe::worker_sync::error_response, 23>(sockets.socket_b());
	EXPECT_EQ(msg.message, "Invalid type-id");
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_receive_unsupport_message_bad)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = 123,
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);

	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	auto const send_result = codec.send_pending_messages();
	EXPECT_EQ(send_result, msg_handler::io_status::ok);
	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<Pipe::worker_sync::error_response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const msg = receive_message<Pipe::worker_sync::error_response, 23>(sockets.socket_b());
	EXPECT_EQ(msg.message, "Invalid type-id");
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_exception_while_handling_request)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<request, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);
	send_message<4>(request{.value = 356}, sockets.socket_b());

	codec.expected_request_value = 356;
	codec.expected_transaction_id = Pipe::worker_sync::transaction_id{24};
	codec.request_exception_string = "Something went wrong";
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	auto const send_result = codec.send_pending_messages();
	EXPECT_EQ(send_result, msg_handler::io_status::ok);
	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<Pipe::worker_sync::error_response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const msg = receive_message<Pipe::worker_sync::error_response, 28>(sockets.socket_b());
	EXPECT_EQ(msg.message, "Something went wrong");
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_exception_while_handling_notification)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<notification, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);
	send_message<4>(notification{.value = 356}, sockets.socket_b());

	codec.expected_notification_value = 356;
	codec.notification_exception_string = "Something went wrong";
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
	EXPECT_EQ(eh_registry.current_listening_status, Pipe::os_services::fd::activity_status::read_or_write);

	auto const send_result = codec.send_pending_messages();
	EXPECT_EQ(send_result, msg_handler::io_status::ok);
	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<Pipe::worker_sync::error_response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{24});

	auto const msg = receive_message<Pipe::worker_sync::error_response, 28>(sockets.socket_b());
	EXPECT_EQ(msg.message, "Something went wrong");
}


// Handle activity event

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_event_error)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	eh_registry.id_to_remove = Pipe::os_services::fd::event_handler_id{345},
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::error
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_can_read_connection_closed)
{
	signal(SIGPIPE, SIG_IGN);
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	sockets.close_socket_b();
	eh_registry.id_to_remove = Pipe::os_services::fd::event_handler_id{345},
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_can_read_read_request)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<request, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);
	send_message<4>(request{.value = 356}, sockets.socket_b());

	codec.expected_request_value = 356;
	codec.expected_transaction_id = Pipe::worker_sync::transaction_id{24};
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_can_write_connection_closed)
{
	signal(SIGPIPE, SIG_IGN);
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 4367}, Pipe::worker_sync::transaction_id{467});
	sockets.close_socket_b();
	eh_registry.id_to_remove = Pipe::os_services::fd::event_handler_id{345},
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::write
		}
	);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_can_write)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	codec.send(response{.value = 4367}, Pipe::worker_sync::transaction_id{467});
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::write
		}
	);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	auto const message = receive_message<response, 4>(sockets.socket_b());
	EXPECT_EQ(message.value, 4367);
}

TESTCASE(Pipe_worker_fwk_sync_message_channel_handle_fd_activity_can_read_and_write)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<request, msg_channel_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{24}
		},
		sockets.socket_b()
	);
	send_message<4>(request{.value = 356}, sockets.socket_b());

	codec.send(response{.value = 4367}, Pipe::worker_sync::transaction_id{467});
	codec.expected_request_value = 356;
	codec.expected_transaction_id = Pipe::worker_sync::transaction_id{24};
	codec.handle_event(
		msg_channel_traits::sync_fd_activity_event{
			.status = Pipe::os_services::fd::activity_status::read_or_write
		}
	);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<response, msg_channel_traits::outgoing_sync_msg_type>)
	);
	auto const message = receive_message<response, 4>(sockets.socket_b());
	EXPECT_EQ(message.value, 4367);
}
