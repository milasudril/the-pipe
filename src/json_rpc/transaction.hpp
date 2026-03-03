#ifndef PIPE_JSON_RPC_TRANSACTION_HPP
#define PIPE_JSON_RPC_TRANSACTION_HPP

#include <jopp/types.hpp>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <functional>

namespace Pipe::json_rpc
{
	/**
	 * \brief A transaction_id can be to identify a transaction (a request with a pending response)
	 */
	class transaction_id
	{
	public:
		/**
		 * \brief Default constructor
		 */
		constexpr transaction_id() = default;

		/**
		 * \brief Constructs a transaction_id from value
		 */
		constexpr explicit transaction_id(int64_t value):
			m_value{value}
		{
			if(value < 0 || value > jopp::max_safe_integer)
			{ throw std::runtime_error{"JSON-RPC transaction id out of range"}; }
		}

		/**
		 * \brief Returns the value as jopp::number, so it can be written to JSON
		 */
		constexpr jopp::number value() const
		{ return static_cast<jopp::number>(m_value); }

		/**
		 * \brief Post-increments the transaction_id
		 * \note This is useful for generating transaction_id:s
		 */
		[[nodiscard]] constexpr transaction_id next()
		{
			auto ret = *this;
			*this = transaction_id{m_value + 1};
			return ret;
		}

		/**
		 * \brief A transaction_id supports all possible comparisons
		 */
		constexpr auto operator<=>(transaction_id const&) const = default;

	private:
		int64_t m_value{};
	};

	class transaction
	{
	public:
		explicit transaction(std::move_only_function<void(jopp::object&&)>&& on_completed):
			m_on_completed{std::move(on_completed)}
		{}

		void finalize(jopp::object&& obj)
		{
			if(obj.get_field_as<jopp::string>("jsonrpc") != "2.0")
			{ throw std::runtime_error{"Unsupported JSON-RPC version"}; }

			auto result = obj.try_get_field_as<jopp::object>("result");
			auto error = obj.try_get_field_as<jopp::object>("error");
			if(result != nullptr && error != nullptr)
			{ throw std::runtime_error{"Ambiguous JSON-RPC response"}; }

			if(result != nullptr)
			{ m_on_completed(std::move(*result)); }
			else
			{
				auto const code = static_cast<int>(error->get_field_as<jopp::number>("code"));
				auto const& message = error->get_field_as<jopp::string>("message");
				throw std::runtime_error{std::format("JSON-RPC remote error {}: {}", code, message)};
			}
		}

	private:
		std::move_only_function<void(jopp::object&&)> m_on_completed;
	};
}
#endif