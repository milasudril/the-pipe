//@	{"dependencies_extra": [{"ref": "./writer.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_IO_WRITER_HPP
#define PIPE_JSON_IO_WRITER_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <jopp/types.hpp>
#include <jopp/serializer.hpp>
#include <memory>
#include <queue>

namespace Pipe::json_io
{
	class writer
	{
	public:
		struct json_stream_tag{};
		using fd_ready_event = os_services::fd::activity_event<
			json_stream_tag,
			os_services::io::output_file_descriptor_tag
		>;

		using activity_event_handler_registered_event = os_services::fd::activity_event_handler_registered_event<
			json_stream_tag,
			os_services::io::output_file_descriptor_tag
		>;

		explicit writer(size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_output_buffer{std::make_unique<char[]>(buffer_size)},
			m_pending_range{m_output_buffer.get(), buffer_size},
			m_range_to_write{}
		{}

		void handle_event(fd_ready_event const&)
		{
			if(m_pending_range.empty())
			{
				if(!drain_range_to_write())
				{ return; }

				auto const ptr = m_output_buffer.get();
				m_pending_range = std::span{ptr, static_cast<size_t>(std::data(m_range_to_write) - ptr)};
			}

			while(!m_to_serialize.empty())
			{
				auto& item = *m_to_serialize.front();
				auto const serialization_result = item.serialize_to(m_pending_range);
				m_pending_range = std::span{
					serialization_result.ptr, std::data(m_pending_range) + std::size(m_pending_range)
				};
				switch(serialization_result.ec)
				{
					case jopp::serializer_error_code::completed:
						m_to_serialize.pop();
						break;

					case jopp::serializer_error_code::buffer_is_full:
						if(!drain_range_to_write())
						{ return; }
						break;

					case jopp::serializer_error_code::illegal_char_in_string:
						abort();
						break;
				}
			}

			m_registration.event_handler_store->update_listening_status(
				m_registration.event_handler,
				os_services::fd::activity_status::read
			);
		}

		void write(jopp::container&& item_to_write)
		{
			m_to_serialize.push(std::make_unique<serialization_ctxt>(std::move(item_to_write)));
			handle_event(fd_ready_event{
				.status = os_services::fd::activity_status::write
			});
		}

		void handle_event(
			activity_event_handler_registered_event const& event
		)
		{ m_registration = event; }

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_output_buffer;
		activity_event_handler_registered_event m_registration;
		std::span<char> m_pending_range;
		std::span<char const> m_range_to_write;

		bool drain_range_to_write()
		{
			while(!m_range_to_write.empty())
			{
				auto const write_result = os_services::io::write(
					m_registration.fd, std::as_bytes(m_range_to_write)
				);
				auto const bytes_transferred = write_result.bytes_transferred();
				if(bytes_transferred == 0)
				{
					if(write_result.operation_would_have_blocked())
					{ return true; }
					else
					{
						m_registration.event_handler_store->remove(m_registration.id);
						return false;
					}
				}


				m_range_to_write = std::span{std::data(m_range_to_write) + bytes_transferred, bytes_transferred};
			}
			return true;
		}

		class serialization_ctxt
		{
		public:
			serialization_ctxt(serialization_ctxt&&) = delete;
			serialization_ctxt(serialization_ctxt const&) = delete;
			serialization_ctxt& operator=(serialization_ctxt&&) = delete;
			serialization_ctxt& operator=(serialization_ctxt const&) = delete;
			~serialization_ctxt() = default;

			explicit serialization_ctxt(jopp::container&& obj):
				m_object{std::move(obj)},
				m_serializer{m_object}
			{ }

			jopp::serialize_result serialize_to(std::span<char> buffer)
			{ return m_serializer.serialize(buffer); }

		private:
			jopp::container m_object;
			jopp::serializer m_serializer;
		};

		std::queue<std::unique_ptr<serialization_ctxt>> m_to_serialize;
	};
}

#endif