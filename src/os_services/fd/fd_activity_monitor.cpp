//@	{"target":{"name":"fd_activity_monitor.test"}}

#include "./fd_activity_monitor.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(prog_os_services_fd_fd_activity_monitor_make_epoll_instance)
{
	auto const val = prog::os_services::fd::make_epoll_instance();
	EXPECT_NE(val, nullptr);
}