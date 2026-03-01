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
	class transaction_id
	{
	public:
		constexpr transaction_id() = default;
		constexpr explicit transaction_id(int64_t value):
			m_value{value}
		{
			if(value < 0 || value > jopp::max_safe_integer)
			{ throw std::runtime_error{"JSON-RPC transaction id out of range"}; }
		}

		constexpr double value() const
		{ return static_cast<double>(m_value); }

		constexpr transaction_id next()
		{
			auto ret = *this;
			*this = transaction_id{m_value + 1};
			return ret;
		}

		constexpr auto operator<=>(transaction_id const&) const = default;

	private:
		int64_t m_value{};
	};

	inline jopp::object ensure_required_fields(jopp::object&& obj)
	{
		if(!obj.contains("method"))
		{ throw std::runtime_error{"JSON-RPC object is missing mandatory field `method`"}; }

		return std::move(obj);
	}

	class request
	{
	public:
		explicit request(jopp::object&& obj):
			m_value{ensure_required_fields(std::move(obj))}
		{}

		explicit request(transaction_id id, std::string_view method, jopp::object&& params):
		{
			m_value.insert("jsonrpc", "2.0");
			m_value.insert("id", id.value());
			m_value.insert("method", method);
			m_value.insert("params", std::move(obj));
		}

		jopp::object&& take_value()
		{ return std::move(m_value); }

	private:
		transaction_id m_id;
		jopp::object m_value;
	};

	class context
	{
	public:
		std::pair<transaction_id, request> make_request(std::string_view method, jopp::object&& params)
		{
			auto const tx_id = m_transaction_id.next();
			return std::pair{tx_id, request{tx_id, method, std::move(params)}};
		}

	private:
		transaction_id m_transaction_id;
	};

	jopp::object make_response(request&& req)
	{
		auto req_value = req.take_value();
		jopp::object response;
		if(auto id = req_value.find("id"); id != std::end(req_value))
		{ response.insert("id", std::move(id->second)); }
		response.insert("jsonrpc", "2.0");
		return response;
	}

	jopp::object make_response(request&& req, jopp::object&& result)
	{
		auto response = make_response(std::move(req));
		response.insert("result", std::move(result));
		return result;
	}

	template<class T>
	concept exception_like = requires(T const& obj)
	{
		{ obj.what() } -> std::same_as<char const*>;
	};

	template<class ExceptionLike>
	jopp::object make_response(request&& req, ExceptionLike const& exception, int code = -32000)
	{
		auto response = make_response(std::move(req));
		response.insert("code", code);
		response.insert("message", exception.what());
		return response;
	}
}

#endif