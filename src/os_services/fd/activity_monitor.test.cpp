//@	{"target":{"name":"activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/activity_event.hpp"

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

