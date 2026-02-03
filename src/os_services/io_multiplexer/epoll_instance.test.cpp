//@	{"target":{"name":"epoll_instance.test"}}

#include "./epoll_instance.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/utils/utils.hpp"

#include <thread>
#include <testfwk/testfwk.hpp>
#include <mutex>
#include <condition_variable>

TESTCASE(Pipe_os_services_io_multiplexeractivity_status_to_epoll_event)
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

TESTCASE(Pipe_os_services_io_multiplexerepoll_event_to_activity_status)
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

namespace
{
	struct my_epoll_entry_data_status
	{
		size_t dtor_callcount = 0;
		Pipe::os_services::fd::activity_event const* last_activity_event{nullptr};
	};

	class my_epoll_entry_data:public Pipe::os_services::io_multiplexer::epoll_entry_data
	{
	public:
		explicit my_epoll_entry_data(my_epoll_entry_data_status& status):m_status{status}
		{}

		int get_fd_native_handle() const noexcept override
		{ return 34; }

		Pipe::os_services::io_multiplexer::event_handler_id get_id() const noexcept override
		{ return Pipe::os_services::io_multiplexer::event_handler_id{765}; }

		void handle_event(Pipe::os_services::fd::activity_event const& event) override
		{
			m_status.get().last_activity_event = &event;
		}

		~my_epoll_entry_data()
		{ ++m_status.get().dtor_callcount;}

		std::reference_wrapper<my_epoll_entry_data_status> m_status;
	};
}

TESTCASE(Pipe_os_services_io_multiplexerepoll_io_multiplexeractivity_no_valid_epoll_instance)
{
	my_epoll_entry_data_status status;
	my_epoll_entry_data my_data{status};
	Pipe::os_services::io_multiplexer::epoll_fd_activity activity{
		my_data,
		Pipe::os_services::fd::activity_status::read,
		Pipe::os_services::fd::file_descriptor_ref{}
	};

	EXPECT_EQ(status.last_activity_event, nullptr);
	activity.process();
	EXPECT_EQ(status.last_activity_event, &activity);

	try
	{
		activity.update_listening_status(Pipe::os_services::fd::activity_status::write);
		abort();
	} catch (...)
	{ }

	EXPECT_EQ(activity.item_should_be_removed(), false);
	activity.stop_listening();
	EXPECT_EQ(activity.item_should_be_removed(), true);

	EXPECT_EQ(activity.get_activity_status(), Pipe::os_services::fd::activity_status::read);
}

namespace
{
	struct my_io_multiplexeractivity_event_handler_status
	{
		Pipe::os_services::fd::activity_event const* last_activity_event{nullptr};
		Pipe::os_services::fd::file_descriptor_ref fd;
	};

	struct my_io_multiplexeractivity_event_handler
	{
		void handle_event(
			Pipe::os_services::fd::activity_event const& event,
			Pipe::os_services::fd::file_descriptor_ref fd
		)
		{
			status.get().last_activity_event = &event;
			status.get().fd = fd;
		}

		std::reference_wrapper<my_io_multiplexeractivity_event_handler_status> status;
	};

	class my_activity_event:public Pipe::os_services::fd::activity_event
	{
	public:
		void update_listening_status(Pipe::os_services::fd::activity_status) const override
		{}

		void stop_listening() const noexcept override
		{}

		Pipe::os_services::fd::activity_status get_activity_status() const noexcept override
		{ return Pipe::os_services::fd::activity_status::none; }
	};
};

TESTCASE(Pipe_os_services_io_multiplexerepoll_entry_data_impl)
{
	my_io_multiplexeractivity_event_handler_status status;
	auto const testfd = ::dup(STDOUT_FILENO);
	Pipe::os_services::io_multiplexer::epoll_entry_data_impl entry_data{
		my_io_multiplexeractivity_event_handler{status},
		Pipe::os_services::fd::file_descriptor{testfd},
		Pipe::os_services::io_multiplexer::event_handler_id{23}
	};

	EXPECT_EQ(entry_data.get_fd_native_handle(), testfd);
	my_activity_event event{};
	entry_data.handle_event(event);
	EXPECT_EQ(status.last_activity_event, &event)
	EXPECT_EQ(status.fd.native_handle(), testfd);
	EXPECT_EQ(entry_data.get_id(), Pipe::os_services::io_multiplexer::event_handler_id{23});
}

namespace
{
	struct my_client
	{
		void handle_event(
			Pipe::os_services::fd::activity_event const& activity,
			Pipe::os_services::ipc::connected_socket_ref<SOCK_SEQPACKET, sockaddr_un> fd
		)
		{
			if(can_read(activity.get_activity_status()))
			{
				std::array<std::byte, 1024> buffer;
				auto res = Pipe::os_services::io::read(fd, buffer);
				if(res.bytes_transferred() == 0)
				{ activity.stop_listening(); }
				else
				{ Pipe::os_services::io::write(fd, std::span{std::data(buffer), res.bytes_transferred()}); }
			}
		}
	};

	struct my_server_event_handler
	{
		std::reference_wrapper<Pipe::os_services::io_multiplexer::epoll_instance> monitor;

		void handle_event(
			Pipe::os_services::fd::activity_event const& activity,
			Pipe::os_services::ipc::server_socket_ref<SOCK_SEQPACKET, sockaddr_un> fd
		)
		{
			if(can_read(activity.get_activity_status()))
			{
				auto id = monitor.get().add(
					accept(fd),
					Pipe::os_services::fd::activity_status::read,
					my_client{}
				);

				EXPECT_EQ(id, Pipe::os_services::io_multiplexer::event_handler_id{1});
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

TESTCASE(Pipe_os_services_io_multiplexerepoll_instance)
{
	event server_created;
	auto const sockname = Pipe::utils::random_printable_ascii_string(Pipe::utils::num_chars_16_bytes);
	auto const address = Pipe::os_services::ipc::make_abstract_sockaddr_un(sockname);
	std::jthread server_thread{[address, &server_created](){
		Pipe::os_services::io_multiplexer::epoll_instance monitor;
		EXPECT_EQ(
			monitor.add(
				Pipe::os_services::ipc::make_server_socket<SOCK_SEQPACKET>(address, 1024),
				Pipe::os_services::fd::activity_status::read,
				my_server_event_handler{monitor}
			),
			Pipe::os_services::io_multiplexer::event_handler_id{0}
		);

		server_created.raise();

		// Get the connection
		monitor.wait_for_and_distpatch_events();

		// Get the request
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
