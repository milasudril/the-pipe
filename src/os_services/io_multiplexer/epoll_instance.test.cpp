//@	{"target":{"name":"epoll_instance.test"}}

#include "./epoll_instance.hpp"
#include "src/os_services/fd/activity_monitor.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/utils/utils.hpp"

#include <thread>
#include <testfwk/testfwk.hpp>
#include <mutex>
#include <condition_variable>

TESTCASE(Pipe_os_services_io_multiplexer_activity_status_to_epoll_event)
{
	EXPECT_EQ(Pipe::os_services::io_multiplexer::to_epoll_event(Pipe::os_services::fd::activity_status::none), 0);
	EXPECT_EQ(Pipe::os_services::io_multiplexer::to_epoll_event(Pipe::os_services::fd::activity_status::read), EPOLLIN);
	EXPECT_EQ(Pipe::os_services::io_multiplexer::to_epoll_event(Pipe::os_services::fd::activity_status::write), EPOLLOUT);
	EXPECT_EQ(Pipe::os_services::io_multiplexer::to_epoll_event(Pipe::os_services::fd::activity_status::read_or_write), EPOLLIN|EPOLLOUT);

	try
	{
		Pipe::os_services::io_multiplexer::to_epoll_event(static_cast<Pipe::os_services::fd::activity_status>(3567));
		abort();
	}
	catch(...)
	{}
}

TESTCASE(Pipe_os_services_io_multiplexer_epoll_event_to_activity_status)
{
	EXPECT_EQ(
		Pipe::os_services::io_multiplexer::epoll_event_to_activity_status(0),
		Pipe::os_services::fd::activity_status::none
	);
	EXPECT_EQ(
		Pipe::os_services::io_multiplexer::epoll_event_to_activity_status(EPOLLIN),
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(
		Pipe::os_services::io_multiplexer::epoll_event_to_activity_status(EPOLLOUT),
		Pipe::os_services::fd::activity_status::write
	);
	EXPECT_EQ(
		Pipe::os_services::io_multiplexer::epoll_event_to_activity_status(EPOLLOUT|EPOLLIN),
		Pipe::os_services::fd::activity_status::read_or_write
	);
}

TESTCASE(Pipe_os_services_io_multiplexer_epoll_entry_data_header_default_construct)
{
	Pipe::os_services::io_multiplexer::epoll_entry_data_header header;
	EXPECT_EQ(sizeof(header), 32);
	EXPECT_LE(alignof(header), 32);
	EXPECT_NE(header.vtable, nullptr);
}

TESTCASE(Pipe_os_services_io_multiplexer_epoll_entry_data_default_construct_no_crash_in_dtor)
{
	Pipe::os_services::io_multiplexer::epoll_entry_data entry_data;
}

TESTCASE(Pipe_os_services_io_multiplexer_epoll_entry_data_create_and_get_props)
{
	static void const*  object_constructed_from = nullptr;
	static void* object_constructed_at = nullptr;
	static void* object_destroyed_at = nullptr;

	auto my_object = 124;

	Pipe::os_services::ipc::pipe pipe;
	{
		Pipe::os_services::io_multiplexer::epoll_entry_data entry_data{
			Pipe::os_services::fd::activity_monitor::event_handler_info{
				.object_address = {.address = &my_object},
				.object_size = 1,
				.object_alignment = 1,
				.handle_event = {},
				.destroy_event_handler_at = [](void* obj){
					object_destroyed_at = obj;
				},
				.construct_event_handler_at = [](
					Pipe::os_services::fd::activity_monitor::dest_object_location dest,
					Pipe::os_services::fd::activity_monitor::source_object_location src
				){
					object_constructed_from = src.address;
					object_constructed_at = dest.address;
				}
			},
			Pipe::os_services::fd::make_generic_file_descriptor(pipe.take_read_end()),
			Pipe::os_services::fd::event_handler_id{54}
		};
		EXPECT_EQ(object_constructed_from, &my_object);
		EXPECT_NE(object_constructed_at, nullptr);
		EXPECT_NE(object_constructed_at, &my_object);
		EXPECT_EQ(entry_data.get_event_handler_ptr(), entry_data.get_header_ptr() + 1);
	}
	EXPECT_EQ(object_destroyed_at, object_constructed_at);
}

namespace
{
	struct my_client
	{
		using event_type = Pipe::os_services::fd::activity_event<
			void,
			Pipe::os_services::ipc::connected_socket_tag<SOCK_SEQPACKET, sockaddr_un>
		>;

		void handle_event(Pipe::os_services::fd::activity_monitor& source, event_type const& event)
		{
			if(can_read(event.status))
			{
				std::array<std::byte, 1024> buffer;
				auto res = Pipe::os_services::io::read(event.fd, buffer);
				if(res.bytes_transferred() == 0)
				{
					source.remove(event.event_handler);
					return;
				}
				m_data_to_send = std::vector(std::begin(buffer),  std::begin(buffer) + res.bytes_transferred());
				source.update_listening_status(event.fd, Pipe::os_services::fd::activity_status::write);
			}
			else
			{
				Pipe::os_services::io::write(event.fd, m_data_to_send);
				source.update_listening_status(event.fd, Pipe::os_services::fd::activity_status::read);
			}
		}

		std::vector<std::byte> m_data_to_send;
	};
	struct my_server_event_handler
	{
		using event_type = Pipe::os_services::fd::activity_event<
			void,
			Pipe::os_services::ipc::server_socket_tag<SOCK_SEQPACKET, sockaddr_un>
		>;

		void handle_event(
			Pipe::os_services::fd::activity_monitor& monitor,
			event_type const& event
		)
		{
			if(can_read(event.status))
			{
				auto const id  = monitor.add<void>(
					my_client{},
					accept(event.fd),
					Pipe::os_services::fd::activity_status::read
				);
				EXPECT_EQ(id, Pipe::os_services::fd::event_handler_id{1});
			}
		}
	};

	class event
	{
	public:
		void wait()
		{
			std::unique_lock lock{m_mtx};
			m_cv.wait(lock, [this](){
				return m_raised;
			});
			m_raised = false;
		}

		void raise()
		{
			std::lock_guard lock{m_mtx};
			m_raised = true;
			m_cv.notify_one();
		}

	private:
		std::mutex m_mtx;
		std::condition_variable m_cv;
		bool m_raised{false};
	};
}

TESTCASE(Pipe_os_services_io_multiplexer_epoll_instance)
{
	event server_created;
	auto const sockname = Pipe::utils::random_printable_ascii_string(Pipe::utils::num_chars_16_bytes);
	auto const address = Pipe::os_services::ipc::make_abstract_sockaddr_un(sockname);
	std::jthread server_thread{[address, &server_created](){
		Pipe::os_services::io_multiplexer::epoll_instance monitor;
		EXPECT_EQ(
			monitor.add<void>(
				my_server_event_handler{},
				Pipe::os_services::ipc::make_server_socket<SOCK_SEQPACKET>(address, 1024),
				Pipe::os_services::fd::activity_status::read
			),
			Pipe::os_services::fd::event_handler_id{0}
		);

		server_created.raise();

		// Get the connection
		monitor.wait_for_and_distpatch_events();

		// Get the request
		monitor.wait_for_and_distpatch_events();

		// Get the response
		monitor.wait_for_and_distpatch_events();

		// Get the close
		monitor.wait_for_and_distpatch_events();
	}};

	server_created.wait();

	auto const connected_socket = Pipe::os_services::ipc::make_connection<SOCK_SEQPACKET>(address);
	auto const write_result = Pipe::os_services::io::write(
		connected_socket.get(),
		std::as_bytes(std::span{std::string_view{"Hello, World"}})
	);
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	std::array<char, 13> buffer{};
	auto const read_result = Pipe::os_services::io::read(
		connected_socket.get(),
		std::as_writable_bytes(std::span{std::data(buffer), std::size(buffer)})
	);
	EXPECT_EQ(read_result.bytes_transferred(), 12);
}
