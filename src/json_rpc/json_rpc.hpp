#ifndef PIPE_JSON_RPC_HPP
#define PIPE_JSON_RPC_HPP

#include "./transaction.hpp"
#include "./request.hpp"
#include "src/utils/utils.hpp"

#include <deque>
#include <stdexcept>
#include <jopp/types.hpp>

/**
 * \brief Support for JSON-RPC
 */
namespace Pipe::json_rpc
{
	/**
	 * \brief A context holds a transaction_id, that is used to generate requests. Thus, a context
	 *        can act as a request factory
	 */
	class context
	{
	public:
		/**
		 * \brief Creates a new request, given method and params
		 */
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
		jopp::object error;
		error.insert("code", static_cast<jopp::number>(code));
		error.insert("message", exception.what());
		response.insert("error", std::move(error));
		return response;
	}
}

#endif