#ifndef PIPE_JSON_RPC_REQUEST_HPP
#define PIPE_JSON_RPC_REQUEST_HPP

#include "./transaction.hpp"
#include "src/utils/utils.hpp"

#include <jopp/types.hpp>
#include <utility>

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
	concept request = requires(T&& obj, jopp::value&& resp_val){
		/**
		 * \brief The name of method that corresponds to T
		 */
		{ request_traits<T>::method } -> std::convertible_to<char const*>;

		/**
		 * \brief Converts resp_val to a result (which must not be void)
		 */
		{ request_traits<T>::make_result(std::move(resp_val)) } -> different_from<void>;
	}
	&& (std::is_empty_v<T> || has_params_to_from_jopp_object<T>);

	template<class ResponseType, class RequestType>
	concept has_result_to_from_jopp_object = requires(ResponseType&& recv_result, jopp::value&& send_result)
	{
		/**
		 * \brief Converts send_result to a some result, for use when processing the response
		 */
		{ request_traits<RequestType>::make_result(std::move(send_result)) } -> std::same_as<ResponseType>;

		/**
		 * \brief Converts recv_result to a jopp::object for use when sending the response
		 */
		{ request_traits<RequestType>::result_to_jopp_object(std::move(recv_result)) } -> std::same_as<jopp::object>;
	};

	template<class ResponseType, class RequestType>
	concept response = std::is_empty_v<ResponseType> || has_result_to_from_jopp_object<ResponseType, RequestType>;

	template<class Handler, class RequestType>
	concept request_handler = request<RequestType> && requires(Handler&& handler, RequestType&& request)
	{
		{ utils::unwrap(std::forward<Handler>(handler)).handle_request(std::move(request)) } -> response<RequestType>;
	};

	template<request RequestType>
	auto make_request(jopp::object&& request_val)
	{
		if constexpr (std::is_empty_v<RequestType>)
		{ return RequestType{}; }
		else
		{
			return request_traits<RequestType>::make_params(
				std::move(request_val)
			);
		}
	}

	/**
	 * \brief Creates a response object, given a wrapped_request
	 *
	 * Using this function ensures that the response inherits the transaction_id from the wrapped_request.
	 * Also, the field "jsonrpc" is added for better conformance.
	 */
	inline jopp::object make_response(wrapped_request const& req)
	{
		auto const& req_value = req.value();
		jopp::object response;
		if(auto id = req_value.find("id"); id != std::end(req_value))
		{
			id->second.visit([&response]<class T>(T const& item){
				if constexpr(std::is_same_v<T, jopp::string> || std::is_same_v<T, jopp::number>)
				{ response.insert("id", item); }
				else
				{ throw std::runtime_error{"Request id has wrong type"}; }
			});
		}
		response.insert("jsonrpc", "2.0");
		return response;
	}

	/**
	 * \brief Creates a response object, given a wrapped_request and its result
	 */
	inline jopp::object make_response(wrapped_request const& req, jopp::object&& result)
	{
		auto response = make_response(req);
		response.insert("result", std::move(result));
		return response;
	}

	/**
	 * \brief Something that looks like an exception
	 */
	template<class T>
	concept exception_like = requires(T const& obj)
	{
		{ obj.what() } -> std::same_as<char const*>;
	};

	/**
	 * \brief Creates a response object, given a wrapped_request and some exception-like object
	 */
	template<exception_like T>
	inline jopp::object make_response(wrapped_request const& req, T const& exception, int code = -32000)
	{
		auto response = make_response(req);
		jopp::object error;
		error.insert("code", static_cast<jopp::number>(code));
		error.insert("message", exception.what());
		response.insert("error", std::move(error));
		return response;
	}

	struct received_request
	{
		std::string method;
		jopp::object params;
		jopp::value id;
	};

	[[nodiscard]] inline jopp::object make_response(jopp::value&& id)
	{
		jopp::object response;
		if(id.get_if<jopp::null>() == nullptr)
		{ response.insert("id", std::move(id)); }
		response.insert("jsonrpc", "2.0");
		return response;
	}

	[[nodiscard]] inline jopp::object make_response(jopp::value&& id, jopp::object&& response)
	{
		auto ret = make_response(std::move(id));
		ret.insert("response", std::move(response));
		return ret;
	}

	template<request RequestType, request_handler<RequestType> RequestHandler>
	[[nodiscard]] jopp::object dispatch_request(received_request&& request, RequestHandler&& handler)
	{
		using response_type = decltype(
			utils::unwrap(std::forward<RequestHandler>(handler)).handle_request(
				std::declval<RequestType>()
			)
		);

		if constexpr(std::is_empty_v<response_type>)
		{
			std::ignore = utils::unwrap(std::forward<RequestHandler>(handler)).handle_request(
				make_request<RequestType>(std::move(request.params))
			);
			return make_response(std::move(request.id), jopp::object{});
		}
		else
		{
			return make_response(
				std::move(request.id),
				request_traits<RequestType>::result_to_jopp_object(
					utils::unwrap(std::forward<RequestHandler>(handler)).handle_request(
						make_request<RequestType>(std::move(request.params))
					)
				)
			);
		}
	}

	struct received_notification
	{
		std::string method;
		jopp::object params;
	};

	struct message_handling_error
	{
		std::string message;
		jopp::value id;
	};

	inline jopp::object make_response(message_handling_error&& err, int code = -32000)
	{
		auto ret = make_response(std::move(err.id));
		jopp::object error;
		error.insert("message", std::move(err.message));
		error.insert("code", jopp::value{static_cast<double>(code)});
		ret.insert("error", std::move(error));
		return ret;
	}

	inline jopp::value copy_id(jopp::value const& val)
	{
		return val.visit(
			utils::overload{
				[](auto const&) -> jopp::value {
					throw std::runtime_error{"Field `id` has an unsupported type"};
				},
				[](jopp::number val) {
					return jopp::value{val};
				},
				[](jopp::string const& val) {
					return jopp::value{val};
				}
			}
		);
	}

	template<class Func, class ErrorHandler>
	void handle_message(jopp::object&& obj, Func&& func, ErrorHandler&& on_error)
	{
		auto const i = obj.find("id");
		auto const is_request = (i != std::end(obj));
		auto id = is_request? std::move(i->second) : jopp::value{};

		try
		{
			jopp::object* params = nullptr;
			auto params_entry = obj.find("params");
			if(params_entry != std::end(obj))
			{
				params = params_entry->second.get_if<jopp::object>();
				if(params == nullptr)
				{ throw std::runtime_error{"The field `params` has wrong type"}; }
			}

			if(is_request)
			{
				std::forward<Func>(func)(
					received_request{
						.method = std::move(obj.get_field_as<jopp::string>("method")),
						.params = (params == nullptr)? jopp::object{} : std::move(*params),
						.id = copy_id(id)
					}
				);
			}
			else
			{
				std::forward<Func>(func)(
					received_notification{
						.method = std::move(obj.get_field_as<jopp::string>("method")),
						.params = (params == nullptr)? jopp::object{} : std::move(*params)
					}
				);
			}
		}
		catch(std::exception const& err)
		{
			std::forward<ErrorHandler>(on_error)(
				message_handling_error{
					.message = err.what(),
					.id = std::move(id)
				}
			);
		}
	}
	template<class Func, class ErrorHandler>
	void handle_message(jopp::array&& reqs, Func func, ErrorHandler on_error)
	{
		for(jopp::value& item: std::move(reqs))
		{
			item.visit([func = func, on_error = on_error]<class T>(T&& obj) mutable {
				if constexpr(std::is_same_v<T, jopp::object>)
				{ handle_message(std::forward<T>(obj), std::move(func), std::move(on_error)); }
				else
				{
					std::move(on_error)(
						json_rpc::message_handling_error{
							.message = "JSON-RPC messages must be objects",
							.id = jopp::value{}
						}
					);
				}
			});
		}
	}
}
#endif