#ifndef PIPE_OS_SERVICES_FD_MESSAGE_SENDER_HPP
#define PIPE_OS_SERVICES_FD_MESSAGE_SENDER_HPP

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"

namespace Pipe::os_services::io
{
	class message_sender
	{
	public:
		explicit message_sender(
			output_file_descriptor_ref fd,
			fd::activity_event_handler_store& store
		):
			m_fd{fd},
			m_event_handler_store{store}
		{}

		void set_buffer(std::span<std::byte const> new_buffer)
		{
			m_output_buffer  = new_buffer;
			m_write_ptr = std::begin(new_buffer);
		}

		size_t send()
		{
			while(m_write_ptr != std::end(m_output_buffer))
			{
				auto const result = write(m_fd, m_output_buffer);
				if(result.operation_would_have_blocked())
				{
					// FIXME: This is not possible, since the complete event handler object is not available
					m_event_handler_store.get().update_listening_status(m_fd, fd::activity_status::read_or_write);
					return m_write_ptr - std::begin(m_output_buffer);
				}

				if(result.bytes_transferred() == 0)
				{
					m_event_handler_store.get().remove(m_id);
					// TODO: Caller may need to know that file was closed
					return m_write_ptr - std::begin(m_output_buffer);
				}
				m_write_ptr += result.bytes_transferred();
			}
			m_write_ptr = std::begin(m_output_buffer);
			return m_write_ptr - std::begin(m_output_buffer);
		}

	private:
		std::span<std::byte const> m_output_buffer;
		std::span<std::byte const>::iterator m_write_ptr;
		io::output_file_descriptor_ref m_fd;
		fd::event_handler_id m_id{};
		std::reference_wrapper<fd::activity_event_handler_store> m_event_handler_store;
	};
}

#endif