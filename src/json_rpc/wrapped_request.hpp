#ifndef PIPE_JSON_RPC_WRAPPED_REQUEST_HPP
#define PIPE_JSON_RPC_WRAPPED_REQUEST_HPP

#include "./transaction.hpp"

#include <jopp/types.hpp>

namespace Pipe::json_rpc
{
	/**
	 * \brief Ensures that obj has all fields required by this JSON-RPC implementation, and throws
	 * and exception if it does not
	 */
	inline jopp::object ensure_required_fields(jopp::object&& obj)
	{
		if(!obj.contains("method"))
		{ throw std::runtime_error{"JSON-RPC object is missing mandatory field `method`"}; }

		auto const i = obj.find("id");
		if(i != std::end(obj))
		{
			auto const& val = i->second;
			if(val.get_if<jopp::number>() == nullptr && val.get_if<jopp::string>() == nullptr)
			{ throw std::runtime_error{"JSON-RPC request id has wrong type"}; }
		}

		return std::move(obj);
	}

	/**
	 * \brief A representation of a JSON-RPC request
	 */
	class wrapped_request
	{
	public:
		/**
		 * \brief Constructs a wrapped_request from a jopp::object, which is assumed to contain the entire
		 *        wrapped_request
		 */
		explicit wrapped_request(jopp::object&& obj):
			m_value{ensure_required_fields(std::move(obj))}
		{}

		/**
		 * \brief Constructs a wrapped_request from an id, a method name, and a set of parameters
		 */
		explicit wrapped_request(transaction_id id, std::string&& method, jopp::object&& params)
		{
			m_value.insert("jsonrpc", "2.0");
			m_value.insert("id", id.value());
			m_value.insert("method", std::move(method));
			m_value.insert("params", std::move(params));
		}

		/**
		 * \brief Moves the value out from the wrapped_request
		 */
		jopp::object&& take_value()
		{ return std::move(m_value); }

		jopp::object const& value() const
		{ return m_value; }

		jopp::object& value()
		{ return m_value; }

		/**
		 * \brief Gets the method of the wrapped_request
		 */
		std::string_view method() const
		{ return m_value.get_field_as<jopp::string>("method"); }

	private:
		transaction_id m_id;
		jopp::object m_value;
	};
}
#endif