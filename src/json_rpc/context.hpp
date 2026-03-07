//@	{"dependencies_extra":[{"ref":"./context.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_RPC_CONTEXT_HPP
#define PIPE_JSON_RPC_CONTEXT_HPP

#include "./request.hpp"
#include "./transaction.hpp"

#include "src/utils/utils.hpp"

#include <deque>
#include <jopp/types.hpp>

namespace Pipe::json_rpc
{
	template<class Request>
	struct request_traits
	{};

	class context
	{
	public:
		template<class Receiver, class Callback>
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
				.write(request{tx_id, std::move(method), std::move(params)}.take_value());
			request_has_been_sent = true;
		}

		template<class Receiver, class Request, class Callback>
		void send_request(Receiver&& receiver, Request&& request, Callback&& callback)
		{
			send_request(
				std::forward<Receiver>(receiver),
				request_traits<Request>::method,
				request_traits<Request>::params(std::forward<Request>(request)),
				[cb = std::forward<Callback>(callback)](jopp::value&& response){
					cb(request_traits<Request>::make_response(std::move(response)));
				}
			);
		}

		template<class Tag = void, class NotificationHandler>
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

		template<class NotificationHandler>
		void handle_messages(jopp::array&& object, NotificationHandler notification_handler)
		{
			for(auto&& item : object)
			{
				handle_messages(std::move(item.get<jopp::object>()), std::ref(notification_handler));
			}
		}

		template<class NotificationHandler>
		void handle_messages(jopp::container&& obj, NotificationHandler&& notification_handler)
		{
			std::move(obj).visit(
				[
					this,
					notification_handler = std::forward<NotificationHandler>(notification_handler)
				](auto&& item) mutable{
					handle_messages(std::move(item), std::move(notification_handler));
				}
			);
		}

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