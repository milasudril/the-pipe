#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <cstdlib>
#include <string>
#include <cstdint>
#include <variant>
#include <span>
#include <cstring>
#include <optional>
#include <utility>
#include <functional>

namespace Pipe::worker_sync
{
	class transaction_id
	{
	public:
		static constexpr uint64_t validity_mask = 0x1;

		constexpr transaction_id() = default;

		constexpr explicit transaction_id(uint64_t value):
			m_value{(value<<1) | validity_mask}
		{}

		constexpr transaction_id next()
		{
			auto const ret = *this;
			m_value += 2;
			return ret;
		}

		constexpr auto value() const
		{ return m_value >> 1; }

		constexpr auto is_valid() const
		{ return static_cast<bool>(m_value & validity_mask); }

		constexpr auto operator<=>(transaction_id const&) const = default;

	private:
		uint64_t m_value{};
	};

	static_assert(std::is_trivially_copyable_v<transaction_id>);

	struct port_activity_subscription_request
	{
		std::string server_portname;

		std::string& content()
		{ return server_portname; }

		std::string const& content() const
		{ return server_portname; }
	};

	class port_activity_subscription_id
	{
	public:
		constexpr port_activity_subscription_id() = default;

		constexpr explicit port_activity_subscription_id(uint64_t value):
			m_value{value}
		{}

		constexpr port_activity_subscription_id next()
		{
			auto const ret = *this;
			++m_value;
			return ret;
		}

		constexpr auto value() const
		{ return m_value; }

		constexpr auto operator<=>(port_activity_subscription_id const&) const = default;

	private:
		uint64_t m_value{};
	};

	struct port_activity_unsubscription
	{ port_activity_subscription_id id; };

	struct client_ready_event
	{ port_activity_subscription_id id; };

	using client_to_server_message = std::variant<
		port_activity_subscription_request,
		port_activity_unsubscription,
		client_ready_event
	>;

	struct error_response
	{
		std::string message;

		std::string const& content() const
		{ return message; }

		std::string& content()
		{ return message; }
	};

	struct port_activity_subscription_response
	{ port_activity_subscription_id id; };

	struct port_activity_unsubscription_response
	{ port_activity_subscription_id id; };

	struct data_ready_event
	{ port_activity_subscription_id id; };

	using server_to_client_message = std::variant<
		data_ready_event,
		error_response,
		port_activity_subscription_response,
		port_activity_unsubscription_response
	>;

	struct msg_header
	{
		uint64_t msg_id;
		transaction_id tx_id;
	};

	class decoder_base
	{
	public:
		template<class Self, class Func, class ErrorHandler>
		size_t decode_and_dispatch(
			this Self&& self,
			std::span<std::byte const> src,
			Func&& func,
			ErrorHandler&& on_error
		)
		{
			auto const ret = self.decode(src);
			if(self.completed())
			{
				try
				{ std::forward<Func>(func)(std::move(self.get_value())); }
				catch(std::exception const& err)
				{
					std::forward<ErrorHandler>(on_error)(
						worker_sync::error_response{
							.message = err.what()
						}
					);
				}
			}
			return ret;
		}
	};

	template<class T>
	class decoder
	{};

	template<class T>
	class encoder{};

	template<class T>
	requires(std::is_trivially_copyable_v<T>)
	class decoder<T>:public decoder_base
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

	template<class T>
	concept decodable_string_message = requires(T& obj)
	{
		{ obj.content() } ->std::same_as<std::string&>;
	};

	template<decodable_string_message Msg>
	class decoder<Msg>:public decoder_base
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
						m_current_object.content().resize(header.string_length);
						m_header_decoder.reset();
						m_write_offset = 0;
					}
				}

				if(!m_header_decoder.has_value()) [[likely]]
				{
					auto& content = m_current_object.content();
					auto const bytes_left = std::size(content) - m_write_offset;
					auto const bytes_to_copy = std::min(bytes_left, std::size(buffer));
					auto const ptr = reinterpret_cast<std::byte*>(std::data(content)) + m_write_offset;
					m_write_offset += bytes_to_copy;
					bytes_consumed += bytes_to_copy;
					memcpy(ptr, std::data(buffer), bytes_to_copy);
					buffer = std::span{std::begin(buffer) + bytes_to_copy, std::end(buffer)};

					if(m_write_offset == std::size(content))
					{ m_completed = true; }
				}
			}

			return bytes_consumed;
		}

		bool completed() const
		{ return m_completed; }

		auto& get_value()
		{ return m_current_object; }

	private:
		struct header
		{ uint64_t string_length; };

		Msg m_current_object;
		bool m_completed = false;
		size_t m_write_offset{};
		std::optional<decoder<header>> m_header_decoder{decoder<header>{}};
	};

	template<class T>
	concept encodable_string_message = requires(T const& obj)
	{
		{obj.content()} -> std::same_as<std::string const&>;
	};

	template<encodable_string_message Msg>
	class encoder<Msg>
	{
	public:
		explicit encoder(Msg&& msg):
			m_current_object{std::move(msg)},
			m_header_encoder{
				encoder<header>{
					header{
						.string_length = std::size(m_current_object.content())
					}
				}
			}
		{}

		size_t encode(std::span<std::byte> buffer)
		{
			size_t bytes_written = 0;
			while(!buffer.empty() && !m_completed)
			{
				if(m_header_encoder.has_value()) [[unlikely]]
				{
					auto const res = m_header_encoder->encode(buffer);
					bytes_written += res;
					buffer = std::span{std::begin(buffer) + res, std::end(buffer)};
					if(m_header_encoder->completed())
					{
						m_header_encoder.reset();
						m_read_offset= 0;
					}
				}

				if(!m_header_encoder.has_value()) [[likely]]
				{
					auto& content = std::as_const(m_current_object).content();
					auto const bytes_left = std::size(content) - m_read_offset;
					auto const bytes_to_copy = std::min(bytes_left, std::size(buffer));
					auto const ptr = reinterpret_cast<std::byte const*>(std::data(content)) + m_read_offset;
					m_read_offset += bytes_to_copy;
					bytes_written += bytes_to_copy;
					memcpy(std::data(buffer), ptr, bytes_to_copy);
					buffer = std::span{std::begin(buffer) + bytes_to_copy, std::end(buffer)};

					if(m_read_offset == std::size(content))
					{ m_completed = true; }
				}
			}
			return bytes_written;
		}

		bool completed() const
		{ return m_completed; }


	private:
		struct header
		{ uint64_t string_length; };

		Msg m_current_object;
		bool m_completed{false};
		size_t m_read_offset{};
		std::optional<encoder<header>> m_header_encoder;
	};

	template<class T>
	encoder(T&&) -> encoder<T>;
}

template<>
struct std::hash<Pipe::worker_sync::port_activity_subscription_id>
{
	static constexpr size_t operator()(Pipe::worker_sync::port_activity_subscription_id id)
	{ return std::hash<uint64_t>{}(id.value()); }
};

#endif
