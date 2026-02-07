//@	{"target":{"name": "activity_monitor.test"}}

#include "./activity_monitor.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/utils/utils.hpp"
#include "testfwk/validation.hpp"

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
	class fd_activity_monitor_stub:public Pipe::os_services::fd::activity_monitor
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

		class blob
		{
		public:
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
							.size = sizeof(size_t),
							.alignment = alignof(size_t)
						},
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
				assert(struct_info.offsets[0] == 0);
				assert(struct_info.offsets[1] == sizeof(size_t));

				data = std::make_unique<std::byte[]>(struct_info.total_size);
				new(data.get())size_t(struct_info.offsets[2]);

				eh_info.construct_event_handler_at(
					dest_object_location{data.get() + sizeof(size_t)}, eh_info.object_address
				);

				new(data.get() + struct_info.offsets[2])saved_event_handler_info(
					eh_info.handle_event,
					std::move(fd),
					status,
					id,
					eh_info.destroy_event_handler_at
				);
			}

			auto event_handler_info_offset() const
			{ return *reinterpret_cast<size_t const*>(data.get()); }

			auto get_saved_event_handler_info()
			{
				return reinterpret_cast<saved_event_handler_info*>(data.get() + event_handler_info_offset());
			}

			void* get_event_handler_ptr() const
			{ return data.get() + sizeof(size_t); }

			~blob()
			{
				if(data != nullptr)
				{
					auto const eh_info = get_saved_event_handler_info();
					eh_info->destroy_event_handler_at(get_event_handler_ptr());
					eh_info->~saved_event_handler_info();
				}
			}

		private:
			std::unique_ptr<std::byte[]> data;
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
		void const* get_event_handler_ptr() const
		{
			return obj.get_event_handler_ptr();;
		}

		void trigger()
		{
			auto ehi = obj.get_saved_event_handler_info();
			ehi->handle_event(
				obj.get_event_handler_ptr(),
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
		Pipe::os_services::fd::activity_monitor* expected_activity_monitor = nullptr;
		Pipe::os_services::fd::activity_monitor* called_with_activity_monitor = nullptr;
		Pipe::os_services::fd::new_activity_event<Pipe::os_services::io::input_file_descriptor_tag> saved_event{};

		void handle_event(
			Pipe::os_services::fd::activity_monitor& activity_monitor,
			Pipe::os_services::fd::new_activity_event<Pipe::os_services::io::input_file_descriptor_tag> const& event
		)
		{
			called_with_activity_monitor = &activity_monitor;
			saved_event = event;
		}
	};
}

TESTCASE(Pipe_os_services_fd_activity_monitor_add_fd)
{
	my_event_handler eh;
	fd_activity_monitor_stub monitor;
	eh.expected_activity_monitor = &monitor;
	Pipe::os_services::ipc::pipe my_pipe;
	auto expected_fd = my_pipe.read_end();
	auto const id = monitor.add(std::ref(eh), my_pipe.take_read_end(), Pipe::os_services::fd::activity_status::read);
	EXPECT_EQ(id, Pipe::os_services::fd::event_handler_id{123});
	REQUIRE_EQ(
		*reinterpret_cast<std::byte const* const*>(monitor.get_event_handler_ptr()), static_cast<void*>(&eh)
	);

	EXPECT_NE(eh.called_with_activity_monitor, eh.expected_activity_monitor);
	EXPECT_NE(eh.saved_event.event_handler, id);
	EXPECT_NE(eh.saved_event.fd, expected_fd);
	EXPECT_NE(eh.saved_event.status, Pipe::os_services::fd::activity_status::read);
	monitor.trigger();
	EXPECT_EQ(eh.called_with_activity_monitor, eh.expected_activity_monitor);
	EXPECT_EQ(eh.saved_event.event_handler, id);
	EXPECT_EQ(eh.saved_event.fd, expected_fd);
	EXPECT_EQ(eh.saved_event.status, Pipe::os_services::fd::activity_status::read);
}