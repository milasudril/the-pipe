//@	{"target":{"name":"activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

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
	prog::os_services::fd::epoll_fd_activity activity{
		new my_epoll_entry_data{status},
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

	EXPECT_EQ(status.dtor_callcount, 0);
	activity.close_fd();
	EXPECT_EQ(status.dtor_callcount, 1);

	EXPECT_EQ(activity.get_activity_status(), prog::os_services::fd::activity_status::read);
}
