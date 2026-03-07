//@	{"target":{"name": "./context.o"}}

#include "./context.hpp"

void Pipe::json_rpc::context::handle_response_from_queue(
	transaction_id id,
	jopp::object&& object
)
{
	auto const i = std::ranges::find_if(m_transactions, [id](auto const& item){
		return id == item.id;
	});
	if(i == std::end(m_transactions))
	{ throw std::runtime_error{"JSON-RPC response has an unexpected transaction id"}; }

	utils::at_scope_exit _{[&](){ m_transactions.erase(i); }};
	i->transaction.finalize(std::move(object));
}