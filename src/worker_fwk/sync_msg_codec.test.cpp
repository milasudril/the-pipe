//@	{"target":{"name":"sync_msg_codec.test"}}

#include "./sync_msg_codec.hpp"
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

	struct msg_codec_traits
	{
		using fd_tag = Pipe::os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;
		using incoming_sync_msg_type = std::variant<request, notification>;
		using outgoing_sync_msg_type = std::variant<response, Pipe::worker_sync::error_response>;
		struct client_activity{};
		using sync_fd_activity_event_handler_registred_event =
			Pipe::os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using sync_fd_activity_event = Pipe::os_services::fd::activity_event<client_activity, fd_tag>;
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
		std::optional<Pipe::os_services::fd::activity_status> new_listening_status;

		void remove(Pipe::os_services::fd::event_handler_id id) override
		{
			EXPECT_EQ(id, id_to_remove);
			id_to_remove.reset();
		}

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref,
			Pipe::os_services::fd::activity_status status
		) override
		{
			EXPECT_EQ(status, new_listening_status);
			new_listening_status.reset();
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
		{
			EXPECT_EQ(id_to_remove.has_value(), false);
			EXPECT_EQ(new_listening_status.has_value(), false);
		}
	};

	struct msg_handler:Pipe::worker_fwk::sync_msg_codec<msg_codec_traits>
	{
		using Pipe::worker_fwk::sync_msg_codec<msg_codec_traits>::sync_msg_codec;
		using Pipe::worker_fwk::sync_msg_codec<msg_codec_traits>::handle_message;

		std::optional<int> expected_request_value;
		std::optional<Pipe::worker_sync::transaction_id> expected_transaction_id;
		std::optional<int> expected_notification_value;

		void handle_request(request req, Pipe::worker_sync::transaction_id id)
		{
			EXPECT_EQ(req.value, expected_request_value);
			expected_request_value.reset();
			EXPECT_EQ(id, expected_transaction_id);
			expected_transaction_id.reset();
		}

		void handle_message(notification notification)
		{
			EXPECT_EQ(notification.value, expected_notification_value);
			expected_notification_value.reset();
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

TESTCASE(Pipe_worker_fwk_sync_msg_codec_handle_event_fd_activity_event_handler_registered_event)
{
	event_handler_store eh_registry;
	Pipe::worker_fwk::sync_msg_codec<msg_codec_traits> codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
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

TESTCASE(Pipe_worker_fwk_sync_msg_codec_read_and_dispatch_requests_no_bytes_ready_operation_would_have_blocked)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto const sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::operation_would_have_blocked);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_read_and_dispatch_requests_no_bytes_ready_fd_closed)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
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

TESTCASE(Pipe_worker_fwk_sync_msg_codec_read_and_dispatch_requests_process_request)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<request, msg_codec_traits::incoming_sync_msg_type>,
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

TESTCASE(Pipe_worker_fwk_sync_msg_codec_read_and_dispatch_requests_process_notification)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	send_message<16>(
		Pipe::worker_sync::msg_header{
			.msg_id = Pipe::utils::variant_index_v<notification, msg_codec_traits::incoming_sync_msg_type>,
			.tx_id = Pipe::worker_sync::transaction_id{325}
		},
		sockets.socket_b()
	);
	send_message<4>(notification{.value = 43}, sockets.socket_b());
	codec.expected_notification_value = 43;
	auto const result = codec.read_and_dispatch_requests();
	EXPECT_EQ(result, msg_handler::io_status::ok);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_pending_messages_operation_would_have_blocked)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	{
		std::array<std::byte, 65536> junk{};
		while(!Pipe::os_services::io::write_full(sockets.socket_a(), junk).operation_would_have_blocked());
	}

	eh_registry.new_listening_status = Pipe::os_services::fd::activity_status::read_or_write;
	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::operation_would_have_blocked);

	auto const result = codec.send_pending_messages();
	EXPECT_EQ(result, msg_handler::io_status::operation_would_have_blocked);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_pending_messages_closed_before_send)
{
	signal(SIGPIPE, SIG_IGN);
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	sockets.close_socket_b();
	eh_registry.id_to_remove = Pipe::os_services::fd::event_handler_id{345};
	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::remote_endpoint_closed);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_pending_messages_closed_after_send)
{
	signal(SIGPIPE, SIG_IGN);
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	{
		std::array<std::byte, 65536> junk{};
		while(!Pipe::os_services::io::write_full(sockets.socket_a(), junk).operation_would_have_blocked());
	}

	eh_registry.new_listening_status = Pipe::os_services::fd::activity_status::read_or_write;
	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::operation_would_have_blocked);

	sockets.close_socket_b();
	auto const result = codec.send_pending_messages();
	EXPECT_EQ(result, msg_handler::io_status::remote_endpoint_closed);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_successful_no_more_messages_no_more_bytes_to_write_not_listening_for_write)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::ok);

	auto const header = receive_message<Pipe::worker_sync::msg_header, 16>(sockets.socket_b());
	EXPECT_EQ(
		header.msg_id,
		(Pipe::utils::variant_index_v<response, msg_codec_traits::outgoing_sync_msg_type>)
	);
	EXPECT_EQ(header.tx_id, Pipe::worker_sync::transaction_id{325});
	auto const payload = receive_message<response, 4>(sockets.socket_b());
	EXPECT_EQ(payload.value, 57);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_successful_no_more_messages_no_more_bytes_to_write_is_listening_for_write)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	{
		std::array<std::byte, 65536> junk{};
		while(!Pipe::os_services::io::write_full(sockets.socket_a(), junk).operation_would_have_blocked());
	}

	eh_registry.new_listening_status = Pipe::os_services::fd::activity_status::read_or_write;
	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::operation_would_have_blocked);

	{
		std::array<std::byte, 65536> junk{};
		while(!Pipe::os_services::io::read_full(sockets.socket_b(), junk).operation_would_have_blocked());
	}

	eh_registry.new_listening_status = Pipe::os_services::fd::activity_status::read;
	auto const result = codec.send_pending_messages();
	EXPECT_EQ(result, msg_handler::io_status::ok);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_blocking_no_more_messages_more_bytes_to_write_is_listening_for_write)
{
	event_handler_store eh_registry;
	msg_handler codec{65536};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	auto const i = fd_map.insert(
		std::pair{
			sockets.socket_a().native_handle(),
			fd_props{.bytes_written = 0, .fake_eagain_above = 10}
		}
	).first;

	eh_registry.new_listening_status = Pipe::os_services::fd::activity_status::read_or_write;
	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::operation_would_have_blocked);
	EXPECT_EQ(i->second.bytes_written, 10);
}

TESTCASE(Pipe_worker_fwk_sync_msg_codec_send_short_buffer)
{
	event_handler_store eh_registry;
	msg_handler codec{17};

	auto sockets = make_sockets();
	codec.handle_event(
		msg_codec_traits::sync_fd_activity_event_handler_registred_event{
			.fd = sockets.socket_a(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_registry,
		}
	);

	auto const i = fd_map.insert(
		std::pair{
			sockets.socket_a().native_handle(),
			fd_props{.bytes_written = 0, .fake_eagain_above = 128}
		}
	).first;

	auto const send_res = codec.send(response{.value = 57}, Pipe::worker_sync::transaction_id{325});
	EXPECT_EQ(send_res, msg_handler::io_status::ok);
	EXPECT_EQ(i->second.bytes_written, 20);
}