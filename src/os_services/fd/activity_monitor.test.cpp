//@	{"target":{"name": "activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/utils/utils.hpp"

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
		struct stored_object
		{
			stored_object() = default;

			explicit stored_object(
				event_handler_info const& eh_info,
				Pipe::os_services::fd::file_descriptor fd,
				Pipe::os_services::fd::activity_status status
			)
			{
				auto const struct_info = Pipe::utils::compute_struct_info(
					std::array{
						Pipe::utils::struct_field_info{
							.size = eh_info.object_size,
							.alignment = eh_info.object_alignment
						},
						Pipe::utils::struct_field_info{
							.size = sizeof(eh_info.handle_event),
							.alignment = sizeof(eh_info.handle_event)
						},
						Pipe::utils::struct_field_info{
							.size = sizeof(fd),
							.alignment = alignof(fd)
						},
						Pipe::utils::struct_field_info{
							.size = sizeof(status),
							.alignment = sizeof(status)
						},
						Pipe::utils::struct_field_info{
							.size = sizeof(Pipe::os_services::fd::event_handler_id),
							.alignment = sizeof(Pipe::os_services::fd::event_handler_id)
						}
					}
				);

				data = std::make_unique<std::byte[]>(struct_info.total_size);
				eh_info.vtable.construct_event_handler_at(
					dest_object_location{data.get() + struct_info.offsets[0]}, eh_info.object_address
				);

				printf("%zu\n", struct_info.total_size);
			}

			std::unique_ptr<std::byte[]> data;
		};

		stored_object obj;

		void do_update_listening_status(
			Pipe::os_services::fd::file_descriptor_ref,
			Pipe::os_services::fd::activity_status
		) override
		{}

		void remove(Pipe::os_services::fd::event_handler_id) override
		{}

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const& eh_info,
			Pipe::os_services::fd::file_descriptor fd,
			Pipe::os_services::fd::activity_status activity_status
		) override
		{
			obj = stored_object{eh_info, std::move(fd), activity_status};
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