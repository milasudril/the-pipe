#ifndef PIPE_WORKER_CTL_JSON_RPC_TRAITS_HPP
#define PIPE_WORKER_CTL_JSON_RPC_TRAITS_HPP

#include "src/json_rpc/context.hpp"
#include "src/json_rpc/json_rpc.hpp"
#include "src/worker_ctl/connection.hpp"
#include "src/worker_ctl/worker_application_info.hpp"

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
	};
}

#endif