#ifndef PIPE_WORKER_FWK_SYNC_MESSAGE_CHANNEL_HPP
#define PIPE_WORKER_FWK_SYNC_MESSAGE_CHANNEL_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include "src/utils/variant_utils.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/utils/fifo.hpp"

#include <fcntl.h>
#include <memory>
#include <cstddef>
#include <span>
#include <queue>

namespace Pipe::worker_fwk
{
	template<class SyncMsgCodecTraits>
	class sync_message_channel
	{
	private:
		using traits = SyncMsgCodecTraits;
		using incoming_msg_type = typename traits::incoming_sync_msg_type;
		using outgoing_msg_type = typename traits::outgoing_sync_msg_type;
		using msg_decoder = utils::wrap_variant_element_t<
			utils::variant_push_front_t<incoming_msg_type, worker_sync::msg_header>,
			worker_sync::decoder
		>;
		using msg_encoder = utils::wrap_variant_element_t<
			utils::variant_push_front_t<outgoing_msg_type, worker_sync::msg_header>,
			worker_sync::encoder
		>;
		using transaction_id = worker_sync::transaction_id;
		using fd_activity_event_handler_registred_event = typename traits::sync_fd_activity_event_handler_registred_event;
		using fd_activity_event = typename traits::sync_fd_activity_event;
		using error_handler = typename traits::error_handler;

	public:
		explicit sync_message_channel(size_t buffer_size):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<std::byte[]>(buffer_size)},
			m_output_buffer{std::make_unique<std::byte[]>(buffer_size)}
		{}

		void handle_event(fd_activity_event_handler_registred_event reg_event)
		{
			auto const fd = reg_event.fd.native_handle();
			auto const flags = ::fcntl(fd, F_GETFL);
			assert(flags != -1);
			::fcntl(reg_event.fd.native_handle(), F_SETFL, O_NONBLOCK|flags);
			m_registration = reg_event;

			if(!m_msgs_to_send.empty())
			{ enable_write_listening(); }
		}

		enum class io_status{ok, operation_would_have_blocked, remote_endpoint_closed};

		template<class Self>
		void handle_event(this Self& self, fd_activity_event event)
		{
			assert(self.m_registration.event_handler_store != nullptr);

			if(has_error(event.status)) [[unlikely]]
			{
				self.m_registration.event_handler_store->remove(self. m_registration.id);
				return;
			}

			if(can_read(event.status))
			{
				if(self.read_and_dispatch_requests() == io_status::remote_endpoint_closed)
				{
					self.m_registration.event_handler_store->remove(self. m_registration.id);
					return;
				}
			}

			if(can_write(event.status))
			{
				if(self.send_pending_messages() == io_status::remote_endpoint_closed)
				{
					self.m_registration.event_handler_store->remove(self. m_registration.id);
					return;
				}
			}
		}

		template<class Self>
		[[nodiscard]] io_status read_and_dispatch_requests(this Self& self)
		{
			auto const read_result = Pipe::os_services::io::read_full(
				self.m_registration.fd, std::span{self.m_input_buffer.get(), self.m_buffer_size}
			);
			if(read_result.bytes_transferred() == 0)
			{
				if(read_result.operation_would_have_blocked())
				{ return io_status::operation_would_have_blocked; }
				return io_status::remote_endpoint_closed;;
			}

			auto bytes_to_process = std::span{self.m_input_buffer.get(), read_result.bytes_transferred()};
			while(!bytes_to_process.empty())
			{
				auto const bytes_consumed = std::visit(
					[&self, bytes_to_process]<class Decoder>(Decoder& item) {
						return item.decode_and_dispatch(
							bytes_to_process,
							[&self]<class Msg>(Msg&& item){
								utils::maybe_at_scope_exit reset_decoder{
									[&self](){
										self.m_currently_received_message = {};
									}
								};

								if constexpr(std::is_same_v<std::remove_cvref_t<Msg>, worker_sync::msg_header>)
								{ reset_decoder.reset(); }

								if constexpr(
									requires{
										{self.handle_request(std::forward<Msg>(item), self.m_current_transaction_id)};
									}
								)
								{ self.handle_request(std::forward<Msg>(item), self.m_current_transaction_id); }
								else
								{ self.handle_message(std::forward<Msg>(item)); }
							},
							[&self](worker_sync::error_response&& response) {
								utils::at_scope_exit reset_decoder{
									[&self](){
										self.m_currently_received_message = {};
									}
								};
								self.send(std::move(response), self.m_current_transaction_id);
							}
						);
					},
					self.m_currently_received_message
				);

				bytes_to_process = std::span{
					std::begin(bytes_to_process) + bytes_consumed,
					std::end(bytes_to_process)
				};
			}

			return io_status::ok;
		}

		[[nodiscard]] io_status flush_output_buffer()
		{
			if(m_bytes_to_write.empty())
			{ return io_status::ok; }

			auto const result = os_services::io::write_full(m_registration.fd, m_bytes_to_write);

			m_bytes_to_write = std::span{
				std::begin(m_bytes_to_write) + result.bytes_transferred(),
				std::end(m_bytes_to_write)
			};

			if(!m_bytes_to_write.empty() || result.bytes_transferred() == 0)
			{
				if(result.operation_would_have_blocked())
				{ return io_status::operation_would_have_blocked; }
				return io_status::remote_endpoint_closed;
			}

			return io_status::ok;
		}


		[[nodiscard]] io_status send_pending_messages()
		{
			while(true)
			{
				if(auto const flush_result = flush_output_buffer(); flush_result != io_status::ok)
				{ return flush_result; }

				std::span serialize_into{m_output_buffer.get(), m_buffer_size};
				size_t bytes_ready = 0;
				size_t bytes_left_to_use = m_buffer_size;
				m_msgs_to_send.drain_until_blocked_or_empty(
					[&](auto& item){
						if(serialize_into.empty())
						{ return false; }

						auto const [bytes_written, is_completed] = std::visit(
							[this, serialize_into](auto& item){
								return std::pair{item.encode(serialize_into), item.completed()};
							},
							item
						);

						bytes_left_to_use -= bytes_written;
						bytes_ready += bytes_written;
						serialize_into = std::span{m_output_buffer.get() + bytes_written, bytes_left_to_use};

						return is_completed;
					}
				);

				m_bytes_to_write = std::span{static_cast<std::byte const*>(m_output_buffer.get()), bytes_ready};

				if(m_msgs_to_send.empty() && m_bytes_to_write.empty())
				{
					disable_write_listening();
					return io_status::ok;
				}
			}
		}

		template<class T>
		void send(T&& msg, worker_sync::transaction_id tx_id)
		{
			auto const msg_id = utils::variant_index_v<T, outgoing_msg_type>;
			m_msgs_to_send.push(
				worker_sync::encoder<worker_sync::msg_header>{
					worker_sync::msg_header{
						.msg_id = msg_id,
						.tx_id = tx_id
					}
				}
			);

			m_msgs_to_send.push(
				worker_sync::encoder<std::remove_cvref_t<T>>{std::forward<T>(msg)}
			);

			if(m_registration.is_valid())
			{ enable_write_listening(); }
		}

		size_t num_messages_to_send() const
		{ return std::size(m_msgs_to_send)/2; }

	protected:
		void handle_message(worker_sync::msg_header header)
		{
			m_current_transaction_id = header.tx_id;
			if(header.msg_id == std::numeric_limits<decltype(header.msg_id)>::max())
			{ throw std::runtime_error{"Invalid type-id"}; }
			m_currently_received_message = utils::make_variant<msg_decoder>(header.msg_id + 1);
		}

	private:
		size_t m_buffer_size;
		// Decoder
		std::unique_ptr<std::byte[]> m_input_buffer;
		std::span<std::byte const> m_bytes_left_to_process;
		msg_decoder m_currently_received_message;
		transaction_id m_current_transaction_id;

		// Encoder
		std::unique_ptr<std::byte[]> m_output_buffer;
		std::span<std::byte const> m_bytes_to_write;
		utils::fifo<msg_encoder> m_msgs_to_send;
		bool m_is_listening_for_write{false};

		fd_activity_event_handler_registred_event m_registration;

		void enable_write_listening()
		{
			if(!m_is_listening_for_write)
			{
				m_registration.event_handler_store->update_listening_status(
					m_registration.event_handler,
					os_services::fd::activity_status::read_or_write
				);
				m_is_listening_for_write = true;
			}
		}

		void  disable_write_listening()
		{
			if(m_is_listening_for_write)
			{
				m_registration.event_handler_store->update_listening_status(
					m_registration.event_handler,
					os_services::fd::activity_status::read
				);
				m_is_listening_for_write = false;
			}
		}
	};
}

#endif
