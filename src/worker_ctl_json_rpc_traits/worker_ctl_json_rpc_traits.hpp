#ifndef PIPE_WORKER_CTL_JSON_RPC_TRAITS_HPP
#define PIPE_WORKER_CTL_JSON_RPC_TRAITS_HPP

#include "src/json_rpc/context.hpp"
#include "src/json_rpc/json_rpc.hpp"
#include "src/json_rpc/request.hpp"
#include "src/worker_ctl/connection.hpp"
#include "src/worker_ctl/disconnection.hpp"
#include "src/worker_ctl/worker_application_info.hpp"

#include <expected>

namespace Pipe::json_rpc
{
	template<>
	struct request_traits<worker_ctl::get_worker_application_info>
	{
		static constexpr char const* method = "get_worker_application_info";

		static jopp::object params(worker_ctl::get_worker_application_info)
		{ return jopp::object{}; }

		static worker_ctl::worker_application_info make_response(jopp::value&& val)
		{ return worker_ctl::make_worker_application_info(std::move(val.get<jopp::object>())); }

		static worker_ctl::get_worker_application_info make_request(jopp::object*)
		{ return worker_ctl::get_worker_application_info{}; }
	};

	template<>
	struct request_traits<worker_ctl::input_connection>
	{
		static constexpr char const* method = "connect_input_port";

		static jopp::object params(worker_ctl::input_connection&& conn)
		{ return to_jopp_object(std::move(conn)); }

		static empty_response make_response(jopp::value&&)
		{ return empty_response{}; }

		static worker_ctl::input_connection make_request(jopp::object* obj)
		{
			if(obj == nullptr)
			{ throw std::runtime_error{"Missing mandatory parameters"}; }
			return worker_ctl::make_input_connection(std::move(*obj));
		}
	};

	template<>
	struct request_traits<worker_ctl::output_connection>
	{
		static constexpr const char* method = "connect_output_port";

		static jopp::object params(worker_ctl::output_connection&& conn)
		{ return to_jopp_object(std::move(conn)); }

		static empty_response make_response(jopp::value&&)
		{ return empty_response{}; }

		static worker_ctl::output_connection make_request(jopp::object* obj)
		{
			if(obj == nullptr)
			{ throw std::runtime_error{"Missing mandatory parameters"}; }
			return worker_ctl::make_output_connection(std::move(*obj));
		}
	};

	template<>
	struct request_traits<worker_ctl::input_disconnection>
	{
		static constexpr char const* method = "disconnect_input_port";

		static jopp::object params(worker_ctl::input_disconnection&& conn)
		{ return to_jopp_object(std::move(conn)); }

		static empty_response make_response(jopp::value&&)
		{ return empty_response{}; }

		static worker_ctl::input_disconnection make_request(jopp::object* obj)
		{
			if(obj == nullptr)
			{ throw std::runtime_error{"Missing mandatory parameters"}; }
			return worker_ctl::make_input_disconnection(std::move(*obj));
		}
	};

	template<>
	struct request_traits<worker_ctl::output_disconnection>
	{
		static constexpr char const* method = "disconnect_output_port";

		static jopp::object params(worker_ctl::output_disconnection&& conn)
		{ return to_jopp_object(std::move(conn)); }

		static empty_response make_response(jopp::value&&)
		{ return empty_response{}; }

		static worker_ctl::output_disconnection make_request(jopp::object* obj)
		{
			if(obj == nullptr)
			{ throw std::runtime_error{"Missing mandatory parameters"}; }
			return worker_ctl::make_output_disconnection(std::move(*obj));
		}
	};

	template<>
	struct notification_traits<worker_ctl::output_disconnection>
	{
		static constexpr char const* method = "remote_input_disconnected";

		static jopp::object params(worker_ctl::output_disconnection&& conn)
		{ return to_jopp_object(std::move(conn)); }
	};

	template<>
	struct notification_traits<worker_ctl::input_disconnection>
	{
		static constexpr char const* method = "remote_output_disconnected";

		static jopp::object params(worker_ctl::input_disconnection&& conn)
		{ return to_jopp_object(std::move(conn)); }
	};
}

namespace Pipe::worker_ctl
{
	template<class Handler>
	jopp::object dispatch_request(json_rpc::wrapped_request&& request, Handler&& handler)
	{
		static constexpr std::array supported_methods{
			json_rpc::request_traits<get_worker_application_info>::method
		};

		static constexpr std::array callbacks{
			json_rpc::dispatch_request<get_worker_application_info, Handler>
		};

		static_assert(std::size(callbacks) == std::size(supported_methods));

		auto const i = std::ranges::find(supported_methods, request.method());
		if(i == std::end(supported_methods))
		{ throw std::runtime_error{"Unsupported method"}; }

		auto const index = i - std::begin(callbacks);
		return callbacks[i](std::move(request), std::forward<Handler>(handler));
	}
};

#endif