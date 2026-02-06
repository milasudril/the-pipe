//@	{"target":{"name": "activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"

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

namespace
{
	class my_fd_activity_monitor:public Pipe::os_services::fd::activity_monitor
	{
		void do_update_listening_status(
			Pipe::os_services::fd::file_descriptor_ref,
			Pipe::os_services::fd::activity_status
		) override
		{}

		void remove(Pipe::os_services::fd::event_handler_id) override
		{}

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{
			return Pipe::os_services::fd::event_handler_id{123};
		}
	};

	struct my_event_handler
	{
		void handle_event(
			Pipe::os_services::fd::activity_monitor&,
			Pipe::os_services::fd::new_activity_event<Pipe::os_services::io::input_file_descriptor_tag> const&
		)
		{}
	};
}

TESTCASE(Pipe_os_services_fd_activity_monitor_add_fd)
{
	my_event_handler eh;
	my_fd_activity_monitor monitor;
	Pipe::os_services::ipc::pipe my_pipe;
	auto const id = monitor.add(std::ref(eh), my_pipe.take_read_end(), Pipe::os_services::fd::activity_status::read);
	EXPECT_EQ(id, Pipe::os_services::fd::event_handler_id{123});
}