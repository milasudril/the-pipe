//@	{"dependencies_extra":[{"ref":"./context.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_RPC_CONTEXT_HPP
#define PIPE_JSON_RPC_CONTEXT_HPP

#include "./request.hpp"
#include "./transaction.hpp"

#include "src/utils/utils.hpp"

#include <deque>
#include <jopp/types.hpp>
#include <type_traits>

namespace Pipe::json_rpc
{
	/**
	 * \brief The response type that corresponds to Request, deduced from the return value of
	 *        request_traits<Request>::make_result(jopp::value)
	 */
	template<request Request>
	using response_type = std::invoke_result_t<
		decltype(request_traits<Request>::make_result),
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
		void send_request(Receiver&& receiver, Request&& request, Callback&& callback)
		{
			send_request(
				std::forward<Receiver>(receiver),
				request_traits<Request>::method,
				[&](){
					if constexpr(std::is_empty_v<Request>)
					{ return jopp::object{}; }
					else
					{ return request_traits<Request>::params_to_jopp_object(std::forward<Request>(request)); }
				}(),
				[cb = std::forward<Callback>(callback)](jopp::value&& response){
					cb(request_traits<Request>::make_result(std::move(response)));
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