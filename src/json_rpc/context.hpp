//@	{"dependencies_extra":[{"ref":"./context.o"}]}

#ifndef PIPE_JSON_RPC_CONTEXT_HPP
#define PIPE_JSON_RPC_CONTEXT_HPP

#include "./request.hpp"
#include "./transaction.hpp"

#include "src/utils/utils.hpp"

#include <deque>
#include <jopp/types.hpp>

namespace Pipe::json_rpc
{
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
			receiver.write(request{tx_id, std::move(method), std::move(params)}.take_value());
			request_has_been_sent = true;
		}

		template<class NotificationHandler>
		void handle_response(jopp::object&& object, NotificationHandler&& notification_handler)
		{
			auto const id_pos = object.find("id");
			if(id_pos == std::end(object))
			{
				utils::unwrap(std::forward<NotificationHandler>(notification_handler))
					.handle_json_rpc_notification(std::move(object));
				return;
			}

			if(m_transactions.empty())
			{ throw std::runtime_error{"No JSON-RPC response expected"}; }

			transaction_id const id{static_cast<int>(id_pos->second.get<jopp::number>())};
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
		void handle_response(jopp::array&& object, NotificationHandler notification_handler)
		{
			for(auto&& item : object)
			{
				handle_response(std::move(item).get<jopp::object>(), std::ref(notification_handler));
			}
		}

		template<class NotificationHandler>
		void process_messages(jopp::container&& obj, NotificationHandler&& notification_handler)
		{
			std::move(obj).visit(
				[
					this,
					notification_handler = std::forward<NotificationHandler>(notification_handler)
				](auto&& item) {
					handle_response(std::move(item));
				}
			);
		}

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