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
			m_reminder{},
			m_is_listening(false)
		{}

		void handle_event(fd_ready_event const&);

		void write(jopp::container&& item_to_write)
		{
			m_to_serialize.push(std::make_unique<serialization_ctxt>(std::move(item_to_write)));
			handle_event(fd_ready_event{});
		}

		void handle_event(
			activity_event_handler_registered_event const& event
		)
		{ m_registration = event; }

		auto get_buffer_size() const
		{ return m_buffer_size; }

		auto get_reminder_size() const
		{ return std::size(m_reminder); }

		auto serialization_queue_is_empty() const
		{ return m_to_serialize.empty(); }

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_output_buffer;
		std::span<char const> m_reminder;
		activity_event_handler_registered_event m_registration;
		bool m_is_listening;

		void enable_listening();

		void disable_listening();

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