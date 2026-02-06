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
		struct saved_event_handler_info
		{
			void (*handle_event)(
				void* object,
				activity_monitor& event_source,
				Pipe::os_services::fd::new_activity_event<Pipe::os_services::fd::generic_fd_tag> const& event
			);
			Pipe::os_services::fd::file_descriptor fd;
			Pipe::os_services::fd::activity_status status;
			Pipe::os_services::fd::event_handler_id id;
			void (*destroy_event_handler_at)(void* object);
		};

		struct blob
		{
			blob() = default;

			blob(blob&&) = default;
			blob& operator=(blob&&) = default;

			explicit blob(
				event_handler_info const& eh_info,
				Pipe::os_services::fd::file_descriptor fd,
				Pipe::os_services::fd::activity_status status,
				Pipe::os_services::fd::event_handler_id id
			)
			{
				auto const struct_info = Pipe::utils::compute_struct_info(
					std::array{
						Pipe::utils::struct_field_info{
							.size = eh_info.object_size,
							.alignment = eh_info.object_alignment
						},
						Pipe::utils::struct_field_info{
							.size = sizeof(saved_event_handler_info),
							.alignment = alignof(saved_event_handler_info)
						}
					}
				);

				data = std::make_unique<std::byte[]>(struct_info.total_size);
				eh_info.construct_event_handler_at(
					dest_object_location{data.get() + struct_info.offsets[0]}, eh_info.object_address
				);
				new(data.get() + struct_info.offsets[1])saved_event_handler_info(
					eh_info.handle_event,
					std::move(fd),
					status,
					id,
					eh_info.destroy_event_handler_at
				);

				subobject_offsets = struct_info.offsets;
			}

			auto get_saved_event_handler_info()
			{
				return reinterpret_cast<saved_event_handler_info*>(data.get() + subobject_offsets[1]);
			}

			~blob()
			{
				if(data != nullptr)
				{
					get_saved_event_handler_info()->destroy_event_handler_at(data.get() + subobject_offsets[0]);
					get_saved_event_handler_info()->~saved_event_handler_info();
				}
			}

			std::unique_ptr<std::byte[]> data;
			std::array<size_t, 2>  subobject_offsets{};
		};

		blob obj;

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
			obj = blob{eh_info, std::move(fd), activity_status, Pipe::os_services::fd::event_handler_id{123}};
			return Pipe::os_services::fd::event_handler_id{123};
		}

	public:
		void trigger()
		{
			auto ehi = obj.get_saved_event_handler_info();
			ehi->handle_event(
				obj.data.get() + obj.subobject_offsets[0],
				*this,
				Pipe::os_services::fd::new_activity_event<Pipe::os_services::fd::generic_fd_tag>{
					.fd = ehi->fd.get(),
					.status = ehi->status,
					.event_handler = ehi->id
				}
			);
		}
	};

	struct my_event_handler
	{
		void handle_event(
			Pipe::os_services::fd::activity_monitor&,
			Pipe::os_services::fd::new_activity_event<Pipe::os_services::io::input_file_descriptor_tag> const&
		)
		{
			puts("Hej");
		}
	};
}

TESTCASE(Pipe_os_services_fd_activity_monitor_add_fd)
{
	my_event_handler eh;
	my_fd_activity_monitor monitor;
	Pipe::os_services::ipc::pipe my_pipe;
	auto const id = monitor.add(std::ref(eh), my_pipe.take_read_end(), Pipe::os_services::fd::activity_status::read);
	EXPECT_EQ(id, Pipe::os_services::fd::event_handler_id{123});

	monitor.trigger();
}