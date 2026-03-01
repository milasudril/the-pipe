#ifndef PIPE_JSON_RPC_HPP
#define PIPE_JSON_RPC_HPP

#include <cstdint>
#include <stdexcept>
#include <jopp/types.hpp>

/**
 * \brief Support for JSON-RPC
 */
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

	/**
	 * \brief Ensures that obj has all fields required by this JSON-RPC implementation, and throws
	 * and exception if it does not
	 */
	inline jopp::object ensure_required_fields(jopp::object&& obj)
	{
		if(!obj.contains("method"))
		{ throw std::runtime_error{"JSON-RPC object is missing mandatory field `method`"}; }
		return std::move(obj);
	}

	/**
	 * \brief A representation of a JSON-RPC request
	 */
	class request
	{
	public:
		/**
		 * \brief Constructs a request from a jopp::object, which is assumed to contain the entire
		 *        request
		 */
		explicit request(jopp::object&& obj):
			m_value{ensure_required_fields(std::move(obj))}
		{}

		/**
		 * \brief Constructs a request from an id, a method name, and a set of parameters
		 */
		explicit request(transaction_id id, std::string&& method, jopp::object&& params)
		{
			m_value.insert("jsonrpc", "2.0");
			m_value.insert("id", id.value());
			m_value.insert("method", std::move(method));
			m_value.insert("params", std::move(params));
		}

		/**
		 * \brief Moves the value out from the request
		 */
		jopp::object&& take_value()
		{ return std::move(m_value); }

		std::string_view method() const
		{ return m_value.get_field_as<jopp::string>("method"); }

	private:
		transaction_id m_id;
		jopp::object m_value;
	};

	/**
	 * \brief A context holds a transaction_id, that is used to generate requests. Thus, a context
	 *        can act as a request factory
	 */
	class context
	{
	public:
		std::pair<transaction_id, request> make_request(std::string&& method, jopp::object&& params)
		{
			auto const tx_id = m_transaction_id.next();
			return std::pair{tx_id, request{tx_id, std::move(method), std::move(params)}};
		}

	private:
		transaction_id m_transaction_id;
	};

	/**
	 * \brief Creates a response object, given a request
	 *
	 * Using this function ensures that the response inherits the transaction_id from the request.
	 * Also, the field "jsonrpc" is added for better conformance.
	 */
	inline jopp::object make_response(request&& req)
	{
		auto req_value = req.take_value();
		jopp::object response;
		if(auto id = req_value.find("id"); id != std::end(req_value))
		{ response.insert("id", std::move(id->second)); }
		response.insert("jsonrpc", "2.0");
		return response;
	}

	/**
	 * \brief Creates a response object, given a request and its result
	 */
	inline jopp::object make_response(request&& req, jopp::object&& result)
	{
		auto response = make_response(std::move(req));
		response.insert("result", std::move(result));
		return response;
	}

	template<class T>
	concept exception_like = requires(T const& obj)
	{
		{ obj.what() } -> std::same_as<char const*>;
	};

	/**
	 * \brief Creates a response object, given a request and some exception-like object
	 */
	template<exception_like T>
	inline jopp::object make_response(request&& req, T const& exception, int code = -32000)
	{
		auto response = make_response(std::move(req));
		response.insert("code", static_cast<jopp::number>(code));
		response.insert("message", exception.what());
		return response;
	}
}

#endif