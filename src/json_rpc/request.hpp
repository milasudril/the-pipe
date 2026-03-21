#ifndef PIPE_JSON_RPC_REQUEST_HPP
#define PIPE_JSON_RPC_REQUEST_HPP

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

	template<class T>
	concept convertible_to_jopp_object = requires(T&& obj)
	{
		{to_jopp_object(std::forward<T>(obj))} -> std::same_as<jopp::object>;
	};

	template<class T>
	concept has_params_to_from_jopp_object = requires(T&& send_params, jopp::object&& recv_params)
	{
		/**
		 * \brief Converts send_params to a jopp::object for use when sending the request
		 */
		{ request_traits<T>::params_to_jopp_object(std::forward<T>(send_params)) } -> std::same_as<jopp::object>;

		/**
		 * \brief Converts recv_params to a T for use when processing the request
		 */
		{ request_traits<T>::make_params(std::move(recv_params)) } -> std::same_as<T>;
	};

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
		 * \brief Converts a jopp::value and returns some response object, to be passed to the request
		 * callback in the ongoing transaction.
		 */
		{ request_traits<T>::make_result(std::move(val)) } -> different_from<void>;
	}
	&& (std::is_empty_v<T> || has_params_to_from_jopp_object<T>);

	template<request RequestType, class Handler>
	jopp::object dispatch_request(json_rpc::wrapped_request&& request, Handler&& handler)
	{
		return make_response(
			request,
			to_jopp_object(
				std::forward<Handler>(handler).handle_request(
					[&](){
						if constexpr (std::is_empty_v<RequestType>)
						{ return RequestType{}; }
						else
						{
							return json_rpc::request_traits<RequestType>::make_params(
								std::move(request.value().get_field_as<jopp::object>("params"))
							);
						}
					}()
				)
			)
		);
	}
}
#endif