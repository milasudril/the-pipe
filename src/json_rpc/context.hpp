//@	{"dependencies_extra":[{"ref":"./context.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_RPC_CONTEXT_HPP
#define PIPE_JSON_RPC_CONTEXT_HPP

#include "./wrapped_request.hpp"
#include "./transaction.hpp"

#include "src/utils/utils.hpp"

#include <deque>
#include <jopp/types.hpp>
#include <type_traits>

namespace Pipe::json_rpc
{
	/**
	 * \brief Type trait to be specialized for T to be used as a request
	 */
	template<class Request>
	struct request_traits
	{};

	template<class S, class T>
	constexpr auto different_from_v = !std::is_same_v<S, T>;

	/**
	 * \brief The opposite of std::same_as
	 */
	template<class S, class T>
	concept different_from = different_from_v<S, T>;

	/**
	 * \brief Defines the requirements of a request
	 */
	template<class T>
	concept request = requires(T&& obj, jopp::value&& val){
		/**
		 * \brief The name of method that corresponds to T
		 */
		{ request_traits<T>::method } -> std::convertible_to<char const*>;

		/**
		 * \brief Converts an object of type T into a jopp::object that will be sent as parameters
		 */
		{ request_traits<T>::params(std::forward<T>(obj)) } -> std::same_as<jopp::object>;

		/**
		 * \brief Converts a jopp::value and returns some response object, to be passed to the request
		 * callback
		 */
		{ request_traits<T>::make_response(std::move(val)) } -> different_from<void>;

	};

	/**
	 * \brief The response type that corresponds to Request, deduced from the return value of
	 *        request_traits<Request>::make_respnse(jopp::value)
	 */
	template<request Request>
	using response_type = std::invoke_result_t<
		decltype(request_traits<Request>::make_response),
		jopp::value
	>;

	/**
	 * \brief Defines the requirements of a callback to be used when sending a request with an
	 * explicit method name and an unknown response type
	 */
	template<class T>
	concept raw_response_callback = requires(T&& obj, jopp::value&& val){
		{ std::forward<T>(obj)(std::move(val)) } -> std::same_as<void>;
	};

	/**
	 * \brief Defines the requirements of a receiver for JSON-RPC requests
	 */
	template<class T>
	concept receiver = requires(T&& recv, jopp::object&& obj){
		{ utils::unwrap(std::forward<T>(recv)).write(std::move(obj)) } -> std::same_as<void>;
	};

	/**
	 * \brief Defines the requirements of a callback when sending requests with a deduced
	 * method name and a known response type
	 */
	template<class T, class ResponseType>
	concept response_callback = requires(T&& obj, ResponseType&& resp)
	{
		{ std::forward<T>(obj)(std::forward<ResponseType>(resp)) } -> std::same_as<void>;
	};

	/**
	 * \brief Defines the requirements of a notification_handler, used to process notifications
	 */
	template<class T, class Tag>
	concept notification_handler = requires(T&& handler, jopp::object&& obj)
	{
		{
			utils::unwrap(std::forward<T>(handler))
				.template handle_json_rpc_notification<Tag>(std::move(obj))
		} -> std::same_as<void>;
	};

	/**
	 * \brief A context is used to send request and handling responses
	 */
	class context
	{
	public:
		/**
		 * \brief Sends a request to receiver
		 */
		template<receiver Receiver, raw_response_callback Callback>
		void send_request(
			Receiver&& receiver,
			std::string&& method,
			jopp::object&& params,
			Callback&& callback
		)
		{
			auto const tx_id = m_transaction_id.next();
			m_transactions.push_back(
				tagged_transaction{
					.id = tx_id,
					.transaction = transaction{std::forward<Callback>(callback)}
				}
			);
			auto request_has_been_sent = false;
			utils::at_scope_exit _{
				[&](){
					if(!request_has_been_sent)
					{ m_transactions.pop_back(); }
				}
			};
			utils::unwrap(std::forward<Receiver>(receiver))
				.write(wrapped_request{tx_id, std::move(method), std::move(params)}.take_value());
			request_has_been_sent = true;
		}

		/**
		 * \brief Sends a request to receiver, with automatic conversion to/from jopp types
		 */
		template<
			receiver Receiver,
			request Request,
			response_callback<response_type<Request>> Callback
		>
		void send_request(Receiver&& receiver, Request&& wrapped_request, Callback&& callback)
		{
			send_request(
				std::forward<Receiver>(receiver),
				request_traits<Request>::method,
				request_traits<Request>::params(std::forward<Request>(wrapped_request)),
				[cb = std::forward<Callback>(callback)](jopp::value&& response){
					cb(request_traits<Request>::make_response(std::move(response)));
				}
			);
		}

		/**
		 * \brief Handles the response or notification in object
		 *
		 * Handles the response or notification in object. Notifications are routed to
		 * NotificationHandler::template handle_json_rpc_notification<Tag>(jopp::object&&). Responses
		 * will be routed to the callback associated with the corresponding request
		 *
		 * \note Despite the name, this member function only handles one message. It is named to make
		 *       dispatching from a jopp::container (which may contain an array) work.
		 */
		template<class Tag = void, notification_handler<Tag> NotificationHandler>
		void handle_messages(jopp::object&& object, NotificationHandler&& notification_handler)
		{
			auto const id_pos = object.find("id");
			if(id_pos == std::end(object))
			{
				utils::unwrap(std::forward<NotificationHandler>(notification_handler))
					.template handle_json_rpc_notification<Tag>(std::move(object));
				return;
			}

			if(m_transactions.empty())
			{ throw std::runtime_error{"No JSON-RPC response expected"}; }

			transaction_id const id{id_pos->second.get<jopp::number>()};
			auto& front = m_transactions.front();
			if(front.id == id) [[likely]]
			{
				utils::at_scope_exit _{[&](){ m_transactions.pop_front(); } };
				front.transaction.finalize(std::move(object));
			}
			else
			{ handle_response_from_queue(id, std::move(object)); }
		}

		/**
		 * \brief Dispatches all messages in object
		 */
		template<class NotificationHandler>
		void handle_messages(jopp::array&& array, NotificationHandler notification_handler)
		{
			for(auto&& item : array)
			{
				handle_messages(std::move(item.get<jopp::object>()), std::ref(notification_handler));
			}
		}

		/**
		 * \brief Dispatches all messages in object
		 */
		template<class NotificationHandler>
		void handle_messages(jopp::container&& object, NotificationHandler&& notification_handler)
		{
			std::move(object).visit(
				[
					this,
					notification_handler = std::forward<NotificationHandler>(notification_handler)
				](auto&& item) mutable{
					handle_messages(std::move(item), std::move(notification_handler));
				}
			);
		}

		/**
		 * \brief Gets the number of pending responses
		 */
		size_t num_pending_responses() const
		{ return m_transactions.size(); }

	private:
		void handle_response_from_queue(transaction_id id, jopp::object&& object);

		transaction_id m_transaction_id;
		struct tagged_transaction
		{
			transaction_id id;
			class transaction transaction;
		};

		std::deque<tagged_transaction> m_transactions;
	};
}
#endif