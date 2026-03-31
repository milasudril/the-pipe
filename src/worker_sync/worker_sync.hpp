#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <cstdlib>
#include <string>
#include <cstdint>
#include <variant>
#include <span>
#include <cstring>
#include <optional>

namespace Pipe::worker_sync
{
	struct port_activity_subscription_request
	{
		uint64_t transaction_id;
		std::string server_portname;
	};

	struct port_activity_unsubscription
	{
		uint64_t transaction_id;
		uint64_t subscription_id;
	};

	struct client_ready_event
	{ uint64_t subscription_id; };

	using client_to_server_message = std::variant<
		port_activity_subscription_request,
		port_activity_unsubscription,
		client_ready_event
	>;

	struct port_activity_subscription
	{
		uint64_t transaction_id;
		uint64_t subscription_id;
	};

	struct port_activity_unsubscription_response
	{
		uint64_t transaction_id;
	};

	struct data_ready_event
	{ uint64_t subscription_id; };

	using server_to_client_message = std::variant<
		port_activity_subscription,
		port_activity_unsubscription_response,
		data_ready_event
	>;

	struct msg_header
	{ uint64_t msg_id; };

	template<class T>
	class decoder{};

	template<class T>
	class encoder{};

	template<class T>
	requires(std::is_trivially_copyable_v<T>)
	class decoder<T>
	{
	public:
		size_t decode(std::span<std::byte const> src)
		{
			if(completed())
			{ return 0; }

			auto const bytes_left = sizeof(T) - m_write_offset;
			auto const bytes_to_copy = std::min(bytes_left, std::size(src));
			auto const ptr = reinterpret_cast<std::byte*>(&m_current_object) + m_write_offset;
			m_write_offset += bytes_to_copy;
			memcpy(ptr, std::data(src), bytes_to_copy);
			return bytes_to_copy;
		}

		bool completed() const
		{ return m_write_offset == sizeof(T); }

		T const& get_value() const
		{ return m_current_object; }

	private:
		T m_current_object{};
		size_t m_write_offset{0};
	};

	template<class T>
	requires(std::is_trivially_copyable_v<T>)
	class encoder<T>
	{
	public:
		explicit encoder(T const& object_to_encode):
			m_current_object{object_to_encode}
		{}

		size_t encode(std::span<std::byte> dest)
		{
			auto const bytes_left = sizeof(T) - m_write_offset;
			auto const bytes_to_copy = std::min(bytes_left, std::size(dest));
			auto const ptr = reinterpret_cast<std::byte const*>(&m_current_object) + m_write_offset;
			m_write_offset += bytes_to_copy;
			memcpy(std::data(dest), ptr, bytes_to_copy);
			return bytes_to_copy;
		}

		bool completed() const
		{ return m_write_offset == sizeof(T); }

	private:
		T m_current_object{};
		size_t m_write_offset{0};
	};

	template<>
	class decoder<port_activity_subscription_request>
	{
	public:
		size_t decode(std::span<std::byte const> buffer)
		{
			size_t bytes_consumed = 0;
			while(!buffer.empty() && !m_completed)
			{
				if(m_header_decoder.has_value()) [[unlikely]]
				{
					auto const res = m_header_decoder->decode(buffer);
					bytes_consumed += res;
					buffer = std::span{std::begin(buffer) + res, std::end(buffer)};
					if(m_header_decoder->completed())
					{
						auto const header = m_header_decoder->get_value();
						m_current_object.transaction_id = header.transaction_id;
						m_current_object.server_portname.resize(header.string_length);
						m_header_decoder.reset();
						m_write_offset = 0;
					}
				}

				if(!m_header_decoder.has_value()) [[likely]]
				{
					auto& server_portname = m_current_object.server_portname;
					auto const bytes_left = std::size(server_portname) - m_write_offset;
					auto const bytes_to_copy = std::min(bytes_left, std::size(buffer));
					auto const ptr = reinterpret_cast<std::byte*>(std::data(server_portname)) + m_write_offset;
					m_write_offset += bytes_to_copy;
					bytes_consumed += bytes_to_copy;
					memcpy(ptr, std::data(buffer), bytes_to_copy);
					buffer = std::span{std::begin(buffer) + bytes_to_copy, std::end(buffer)};

					if(m_write_offset == std::size(server_portname))
					{ m_completed = true; }
				}
			}

			return bytes_consumed;
		}

		bool completed() const
		{ return m_completed; }

		port_activity_subscription_request& get_value()
		{ return m_current_object; }

	private:
		struct header
		{
			uint64_t transaction_id;
			uint64_t string_length;
		};

		port_activity_subscription_request m_current_object;
		bool m_completed = false;
		size_t m_write_offset{};
		std::optional<decoder<header>> m_header_decoder{decoder<header>{}};
	};
}

#endif