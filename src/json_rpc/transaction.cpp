//@	{"target":{"name":"transaction.o"}}

#include "./transaction.hpp"
#include <jopp/types.hpp>

void Pipe::json_rpc::transaction::finalize(jopp::object&& response)
{
	if(response.get_field_as<jopp::string>("jsonrpc") != "2.0")
	{ throw std::runtime_error{"Unsupported JSON-RPC version"}; }

	// TODO: According to JSON-RPC 2.0, the result does not have to be an object
	auto result = response.find("result");
	auto error = response.try_get_field_as<jopp::object>("error");

	if(
		   (result == std::end(response) && error == nullptr)
		|| (result != std::end(response) && error != nullptr)
	)
	{
		throw std::runtime_error{
			"A JSON-RPC response must contain either a `result` or an `error` object"
		};
	}

	if(result != std::end(response)) [[likely]]
	{ m_on_completed(std::move(result->second)); }
	else
	{
		auto const code = error->try_get_field_as<jopp::number>("code");
		auto const message = error->try_get_field_as<jopp::string>("message");
		if(code == nullptr || message == nullptr)
		{
			throw std::runtime_error{
				"A JSON-RPC error must consist of a numeric `code` and a string `message`"
			};
		}

		throw std::runtime_error{
			std::format(
				"JSON-RPC remote error {}: {}",
				static_cast<int>(*code),
				*message
			)
		};
	}
}