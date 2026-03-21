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
		{ notification_traits<T>::params_to_jopp_object(std::forward<T>(obj)) } -> std::same_as<jopp::object>;
	};

	/**
	 * \brief Creates a JSON-RPC notification from notification
	 */
	template<notification Notification>
	inline jopp::object make_notification(Notification&& notification)
	{
		return make_notification(
			notification_traits<Notification>::method,
			notification_traits<Notification>::params_to_jopp_object(std::forward<Notification>(notification))
		);
	}
}

#endif