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

	class context_2
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

		bool handle_response(jopp::object&& object)
		{
			auto const id_pos = object.find("id");
			if(id_pos == std::end(object))
			{ return false; }

			if(m_transactions.empty())
			{ throw std::runtime_error{"No JSON-RPC response expected"}; }

			transaction_id const id{static_cast<int>(id_pos->second.get<jopp::number>())};
			auto& front = m_transactions.front();
			if(front.id == id) [[likely]]
			{
				utils::at_scope_exit _{
					[&](){
						m_transactions.pop_front();
					}
				};
				front.transaction.finalize(std::move(object));
				return true;
			}
			auto const i = std::ranges::find_if(m_transactions, [id](auto const& item){
				return id == item.id;
			});
			if(i == std::end(m_transactions))
			{ throw std::runtime_error{"JSON-RPC unexpected transaction id"}; }
			utils::at_scope_exit _{
				[&](){
					m_transactions.erase(i);
				}
			};
			i->transaction.finalize(std::move(object));
			return true;
		}


	private:
		transaction_id m_transaction_id;
		struct tagged_transaction
		{
			transaction_id id;
			class transaction transaction;
		};

		std::deque<tagged_transaction> m_transactions;
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