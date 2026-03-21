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
	struct empty_response{};

	/**
	 * \brief Creates a JSON-RPC notification given method and object
	 */
	inline jopp::object make_notification(std::string&& method, jopp::object&& params)
	{
		jopp::object ret;
		ret.insert("jsonrpc", "2.0");
		ret.insert("method", std::move(method));
		ret.insert("params", std::move(params));
		return ret;
	}

	/**
	 * \brief Type trait to be specialized for T to be used as a notification
	 */
	template<class T>
	struct notification_traits
	{};

	/**
	 * \brief Defines the requirements of a notification
	 */
	template<class T>
	concept notification = requires(T&& obj){
		/**
		 * \brief The name of method that corresponds to T
		 */
		{ notification_traits<T>::method } -> std::convertible_to<char const*>;

		/**
		 * \brief Converts an object of type T into a jopp::object that will be sent as parameters
		 */
		{ notification_traits<T>::params(std::forward<T>(obj)) } -> std::same_as<jopp::object>;
	};

	/**
	 * \brief Creates a JSON-RPC notification from notification
	 */
	template<notification Notification>
	inline jopp::object make_notification(Notification&& notification)
	{
		return make_notification(
			notification_traits<Notification>::method,
			notification_traits<Notification>::params(std::forward<Notification>(notification))
		);
	}

	 /**
	 * \brief Creates a response object, given a wrapped_request
	 *
	 * Using this function ensures that the response inherits the transaction_id from the wrapped_request.
	 * Also, the field "jsonrpc" is added for better conformance.
	 */
	inline jopp::object make_response(wrapped_request const& req)
	{
		auto const& req_value = req.value();
		jopp::object response;
		if(auto id = req_value.find("id"); id != std::end(req_value))
		{
			id->second.visit([&response]<class T>(T const& item){
				if constexpr(std::is_same_v<T, jopp::string> || std::is_same_v<T, jopp::number>)
				{ response.insert("id", item); }
				else
				{ throw std::runtime_error{"Request id has wrong type"}; }
			});
		}
		response.insert("jsonrpc", "2.0");
		return response;
	}

	/**
	 * \brief Creates a response object, given a wrapped_request and its result
	 */
	inline jopp::object make_response(wrapped_request const& req, jopp::object&& result)
	{
		auto response = make_response(req);
		response.insert("result", std::move(result));
		return response;
	}

	/**
	 * \brief Something that looks like an exception
	 */
	template<class T>
	concept exception_like = requires(T const& obj)
	{
		{ obj.what() } -> std::same_as<char const*>;
	};

	/**
	 * \brief Creates a response object, given a wrapped_request and some exception-like object
	 */
	template<exception_like T>
	inline jopp::object make_response(wrapped_request const& req, T const& exception, int code = -32000)
	{
		auto response = make_response(req);
		jopp::object error;
		error.insert("code", static_cast<jopp::number>(code));
		error.insert("message", exception.what());
		response.insert("error", std::move(error));
		return response;
	}
}

#endif