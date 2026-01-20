//@	{"target":{"name":"activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/utils/utils.hpp"

#include <thread>
#include <testfwk/testfwk.hpp>
#include <mutex>
#include <condition_variable>

TESTCASE(prog_os_services_fd_activity_status_to_epoll_event)
{
	EXPECT_EQ(to_epoll_event(prog::os_services::fd::activity_status::none), 0);
	EXPECT_EQ(to_epoll_event(prog::os_services::fd::activity_status::read), EPOLLIN);
	EXPECT_EQ(to_epoll_event(prog::os_services::fd::activity_status::write), EPOLLOUT);
	EXPECT_EQ(to_epoll_event(prog::os_services::fd::activity_status::read_or_write), EPOLLIN|EPOLLOUT);

	try
	{
		to_epoll_event(static_cast<prog::os_services::fd::activity_status>(3567));
		abort();
	}
	catch(...)
	{}
}

TESTCASE(prog_os_services_fd_epoll_event_to_activity_status)
{
	EXPECT_EQ(
		prog::os_services::fd::epoll_event_to_activity_status(0),
		prog::os_services::fd::activity_status::none
	);
	EXPECT_EQ(
		prog::os_services::fd::epoll_event_to_activity_status(EPOLLIN),
		prog::os_services::fd::activity_status::read
	);
	EXPECT_EQ(
		prog::os_services::fd::epoll_event_to_activity_status(EPOLLOUT),
		prog::os_services::fd::activity_status::write
	);
	EXPECT_EQ(
		prog::os_services::fd::epoll_event_to_activity_status(EPOLLOUT|EPOLLIN),
		prog::os_services::fd::activity_status::read_or_write
	);
}

namespace
{
	struct my_epoll_entry_data_status
	{
		size_t dtor_callcount = 0;
		prog::os_services::fd::activity_event const* last_activity_event{nullptr};
	};

	class my_epoll_entry_data:public prog::os_services::fd::epoll_entry_data
	{
	public:
		explicit my_epoll_entry_data(my_epoll_entry_data_status& status):m_status{status}
		{}

		int get_fd_native_handle() const noexcept override
		{ return 34; }

		void handle_event(prog::os_services::fd::activity_event const& event) override
		{
			m_status.get().last_activity_event = &event;
		}

		~my_epoll_entry_data()
		{ ++m_status.get().dtor_callcount;}

		std::reference_wrapper<my_epoll_entry_data_status> m_status;
	};
}

TESTCASE(prog_os_services_fd_epoll_fd_activity_no_valid_epoll_instance)
{
	my_epoll_entry_data_status status;
	my_epoll_entry_data my_data{status};
	prog::os_services::fd::epoll_fd_activity activity{
		my_data,
		prog::os_services::fd::activity_status::read,
		prog::os_services::fd::file_descriptor_ref{}
	};

	EXPECT_EQ(status.last_activity_event, nullptr);
	activity.process();
	EXPECT_EQ(status.last_activity_event, &activity);

	try
	{
		activity.update_listening_status(prog::os_services::fd::activity_status::write);
		abort();
	} catch (...)
	{ }

	EXPECT_EQ(activity.item_should_be_removed(), false);
	activity.stop_listening();
	EXPECT_EQ(activity.item_should_be_removed(), true);

	EXPECT_EQ(activity.get_activity_status(), prog::os_services::fd::activity_status::read);
}

namespace
{
	struct my_fd_activity_event_handler_status
	{
		prog::os_services::fd::activity_event const* last_activity_event{nullptr};
		prog::os_services::fd::file_descriptor_ref fd;
	};

	struct my_fd_activity_event_handler
	{
		void handle_event(
			prog::os_services::fd::activity_event const& event,
			prog::os_services::fd::file_descriptor_ref fd
		)
		{
			status.get().last_activity_event = &event;
			status.get().fd = fd;
		}

		std::reference_wrapper<my_fd_activity_event_handler_status> status;
	};

	class my_activity_event:public prog::os_services::fd::activity_event
	{
	public:
		void update_listening_status(prog::os_services::fd::activity_status) const override
		{}

		void stop_listening() const noexcept override
		{}

		prog::os_services::fd::activity_status get_activity_status() const noexcept override
		{ return prog::os_services::fd::activity_status::none; }
	};
};

TESTCASE(prog_os_services_fd_epoll_entry_data_impl)
{
	my_fd_activity_event_handler_status status;
	auto const testfd = ::dup(STDOUT_FILENO);
	prog::os_services::fd::epoll_entry_data_impl entry_data{
		my_fd_activity_event_handler{status},
		prog::os_services::fd::file_descriptor{testfd},
		prog::os_services::fd::event_handler_id{23}
	};

	EXPECT_EQ(entry_data.get_fd_native_handle(), testfd);
	my_activity_event event{};
	entry_data.handle_event(event);
	EXPECT_EQ(status.last_activity_event, &event)
	EXPECT_EQ(status.fd.native_handle(), testfd);
	EXPECT_EQ(entry_data.id(), prog::os_services::fd::event_handler_id{23});
}

namespace
{
	struct my_client
	{
		void handle_event(
			prog::os_services::fd::activity_event const& activity,
			prog::os_services::ipc::connected_socket_ref<SOCK_SEQPACKET, sockaddr_un> fd
		)
		{
			if(can_read(activity.get_activity_status()))
			{
				std::array<std::byte, 1024> buffer;
				auto res = prog::os_services::io::read(fd, buffer);
				if(res.bytes_transferred() == 0)
				{ activity.stop_listening(); }
				else
				{ prog::os_services::io::write(fd, std::span{std::data(buffer), res.bytes_transferred()}); }
			}
		}
	};

	struct my_server_event_handler
	{
		std::reference_wrapper<prog::os_services::fd::activity_monitor> monitor;

		void handle_event(
			prog::os_services::fd::activity_event const& activity,
			prog::os_services::ipc::server_socket_ref<SOCK_SEQPACKET, sockaddr_un> fd
		)
		{
			if(can_read(activity.get_activity_status()))
			{
				monitor.get().add(
					accept(fd),
					prog::os_services::fd::activity_status::read,
					my_client{}
				);
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

TESTCASE(prog_os_services_fd_activity_monitor)
{
	event server_created;
	auto const sockname = prog::utils::random_printable_ascii_string(prog::utils::num_chars_16_bytes);
	auto const address = prog::os_services::ipc::make_abstract_sockaddr_un(sockname);
	std::jthread server_thread{[address, &server_created](){
		prog::os_services::fd::activity_monitor monitor;
		monitor.add(
			prog::os_services::ipc::make_server_socket<SOCK_SEQPACKET>(address, 1024),
			prog::os_services::fd::activity_status::read,
			my_server_event_handler{monitor}
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

	auto const connected_socket = prog::os_services::ipc::make_connection<SOCK_SEQPACKET>(address);
	auto const write_result = prog::os_services::io::write(
		connected_socket.get(),
		std::as_bytes(std::span{std::string_view{"Hello, World"}})
	);
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	std::array<char, 13> buffer{};
	auto const read_result = prog::os_services::io::read(
		connected_socket.get(),
		std::as_writable_bytes(std::span{std::data(buffer), std::size(buffer)})
	);
	EXPECT_EQ(read_result.bytes_transferred(), 12);
}
