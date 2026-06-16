//@	{"target":{"name": "activity_event_handler_store.test"}}

#include "./activity_event_handler_store.hpp"
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
	class fd_activity_event_handler_store_stub:public Pipe::os_services::fd::activity_event_handler_store
	{
		struct event_handler_vtable
		{
			Pipe::os_services::fd::file_descriptor_deleter<Pipe::os_services::fd::generic_fd_tag> fd_deleter;
			void (*destroy_event_handler_at)(void* object);
		};

		struct saved_event_handler_info
		{
			saved_event_handler_info():vtable{std::make_unique<event_handler_vtable>()}
			{}

			void (*handle_event)(
				void* object,
				Pipe::os_services::fd::activity_event<void, Pipe::os_services::fd::generic_fd_tag> const& event
			);
			Pipe::os_services::fd::file_descriptor_ref fd;
			Pipe::os_services::fd::activity_status status;  // Not used in real version
			Pipe::os_services::fd::event_handler_id id;
			std::unique_ptr<event_handler_vtable> vtable;
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
							.size = sizeof(saved_event_handler_info),
							.alignment = alignof(saved_event_handler_info)
						},
						Pipe::utils::struct_field_info{
							.size = eh_info.object_size,
							.alignment = eh_info.object_alignment
						}
					}
				);
				static_assert(sizeof(saved_event_handler_info)%alignof(std::max_align_t) == 0);
				assert(eh_info.object_alignment <= alignof(std::max_align_t));

				data = std::make_unique<std::byte[]>(struct_info.total_size);
				auto saved_eh_info = new(data.get())saved_event_handler_info;
				saved_eh_info->handle_event = eh_info.handle_activity_event;
				saved_eh_info->vtable->fd_deleter = fd.get_deleter();
				saved_eh_info->vtable->destroy_event_handler_at = eh_info.destroy_event_handler_at;
				saved_eh_info->fd = fd.release();
				saved_eh_info->status = status;
				saved_eh_info->id = id;

				eh_info.construct_event_handler_at(
					dest_object_location{data.get() + sizeof(saved_event_handler_info)}, eh_info.object_address
				);
			}

			auto event_handler_info_offset() const
			{ return *reinterpret_cast<size_t const*>(data.get()); }

			auto get_saved_event_handler_info()
			{
				return reinterpret_cast<saved_event_handler_info*>(data.get());
			}

			void* get_event_handler_ptr() const
			{ return data.get() + sizeof(saved_event_handler_info); }

			~blob()
			{
				if(data != nullptr)
				{
					auto const eh_info = get_saved_event_handler_info();
					eh_info->vtable->destroy_event_handler_at(get_event_handler_ptr());
					eh_info->vtable->fd_deleter(eh_info->fd);
					eh_info->~saved_event_handler_info();
				}
			}

		private:
			std::unique_ptr<std::byte[]> data;
		};

		blob obj;

		void update_listening_status(
			Pipe::os_services::fd::event_handler_cookie,
			Pipe::os_services::fd::activity_status new_status
		) noexcept override
		{
			obj.get_saved_event_handler_info()->status = new_status;
		}

		void remove(Pipe::os_services::fd::event_handler_id) noexcept override
		{}

		std::pair<void*, Pipe::os_services::fd::event_handler_id> do_add(
			event_handler_info const& eh_info,
			Pipe::os_services::fd::file_descriptor fd,
			Pipe::os_services::fd::activity_status activity_status
		) override
		{
			obj = blob{eh_info, std::move(fd), activity_status, Pipe::os_services::fd::event_handler_id{123}};
			return std::pair{obj.get_event_handler_ptr(), Pipe::os_services::fd::event_handler_id{123}};
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
				Pipe::os_services::fd::activity_event<void, Pipe::os_services::fd::generic_fd_tag>{
					.status = ehi->status
				}
			);
		}
	};

	struct my_tag{};

	struct my_event_handler
	{
		Pipe::os_services::fd::activity_event<
			my_tag,
			Pipe::os_services::io::input_file_descriptor_tag
		> saved_event{};

		Pipe::os_services::fd::activity_event_handler_registered_event<
			my_tag,
			Pipe::os_services::io::input_file_descriptor_tag
		> registration;

		void handle_event(
			Pipe::os_services::fd::activity_event<
				my_tag,
				Pipe::os_services::io::input_file_descriptor_tag
			> const& event
		)
		{ saved_event = event; }

		void handle_event(Pipe::os_services::fd::activity_event_handler_registered_event<my_tag, Pipe::os_services::io::input_file_descriptor_tag> const& event)
		{
			registration =event;
		}
	};
}

TESTCASE(Pipe_os_services_fd_activity_event_handler_store_add_fd)
{
	my_event_handler eh;
	fd_activity_event_handler_store_stub monitor;
	Pipe::os_services::ipc::pipe my_pipe;
	auto [saved_ptr, id] = monitor.add<my_tag>(
		std::ref(eh), my_pipe.take_read_end(), Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(id, Pipe::os_services::fd::event_handler_id{123});

	auto const event_handler_ptr = std::bit_cast<std::array<std::byte, 8>>(
		*reinterpret_cast<std::byte const* const*>(monitor.get_event_handler_ptr())
	);
	auto const expected_eh_ptr = std::bit_cast<std::array<std::byte, 8>>(&eh);
	REQUIRE_EQ(event_handler_ptr, expected_eh_ptr);
	EXPECT_EQ(monitor.get_event_handler_ptr(), &saved_ptr.get());

	EXPECT_NE(eh.saved_event.status, Pipe::os_services::fd::activity_status::read);
	monitor.trigger();
	EXPECT_EQ(eh.saved_event.status, Pipe::os_services::fd::activity_status::read);

//
// TODO:
//	monitor.update_listening_status(
//		expected_fd, Pipe::os_services::fd::activity_status::write
//	);
//	monitor.trigger();
//	EXPECT_EQ(eh.saved_event.status, Pipe::os_services::fd::activity_status::write);
}