//@	{"target":{"name": "activity_event.test"}}

#include "./activity_event.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_os_services_fd_activity_status_can_read)
{
	EXPECT_EQ(can_read(Pipe::os_services::fd::activity_status::none), false);
	EXPECT_EQ(can_read(Pipe::os_services::fd::activity_status::read), true);
	EXPECT_EQ(can_read(Pipe::os_services::fd::activity_status::write), false);
	EXPECT_EQ(can_read(Pipe::os_services::fd::activity_status::read_or_write), true);
}

TESTCASE(Pipe_os_services_fd_activity_status_can_write)
{
	EXPECT_EQ(can_write(Pipe::os_services::fd::activity_status::none), false);
	EXPECT_EQ(can_write(Pipe::os_services::fd::activity_status::read), false);
	EXPECT_EQ(can_write(Pipe::os_services::fd::activity_status::write), true);
	EXPECT_EQ(can_write(Pipe::os_services::fd::activity_status::read_or_write), true);
}