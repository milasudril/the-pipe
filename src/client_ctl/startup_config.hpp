#ifndef PIPE_CLIENT_CTL_STARTUP_CONFIG_HPP
#define PIPE_CLIENT_CTL_STARTUP_CONFIG_HPP

#include "src/json_log/item_converter.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"

#include <jopp/types.hpp>
#include <variant>
#include <map>
#include <vector>
#include <filesystem>

namespace Pipe::client_ctl
{
	template<class T>
	struct host_address_type_info
	{};

	using socket_fd_ref = os_services::ipc::connected_socket_ref<SOCK_STREAM, sockaddr_un>;

	template<>
	struct host_address_type_info<socket_fd_ref>
	{
		static constexpr const char* name = "socket_fd";
	};

	inline jopp::number to_jopp_value(socket_fd_ref value)
	{
		return static_cast<jopp::number>(value.native_handle());
	}

	using host_address = std::variant<socket_fd_ref>;

	inline jopp::object to_jopp_object(host_address address)
	{
		return std::visit(
			[]<class T>(T const& object){
				jopp::object ret;
				ret.insert("type", host_address_type_info<T>::name);
				ret.insert("value", to_jopp_value(object));
				return ret;
			},
			address
		);
	}

	template<class T>
	struct operational_mode_info
	{};

	struct host_info
	{
		host_address address;
	};

	template<>
	struct operational_mode_info<host_info>
	{
		static constexpr const char* name = "connected_to_host";
	};

	inline jopp::object to_jopp_object(host_info const& object)
	{
		jopp::object ret;
		ret.insert("address", to_jopp_object(object.address));
		return ret;
	}

	using file_input_port_map = std::map<std::string, std::filesystem::path>;

	inline jopp::object to_jopp_object(file_input_port_map const& object)
	{
		jopp::object ret;
		for(auto const& item: object)
		{ ret.insert(std::string{item.first}, item.second.string()); }
		return ret;
	}

	using file_output_port_map = std::map<std::string, std::vector<std::filesystem::path>>;

	inline jopp::object to_jopp_object(file_output_port_map const& object)
	{
		jopp::object ret;
		for(auto const& item: object)
		{
			jopp::array paths;
			for(auto const& path: item.second)
			{ paths.push_back(path.string()); }

			ret.insert(std::string{item.first}, std::move(paths));
		}
		return ret;
	}

	struct local_config
	{
		file_input_port_map inputs;
		file_output_port_map outputs;
	};

	template<>
	struct operational_mode_info<local_config>
	{
		static constexpr const char* name = "standalone";
	};

	inline jopp::object to_jopp_object(local_config const& cfg)
	{
		jopp::object ret;
		ret.insert("inputs", to_jopp_object(cfg.inputs));
		ret.insert("outputs", to_jopp_object(cfg.outputs));
		return ret;
	}

	using startup_config = std::variant<host_info, local_config>;

	inline jopp::object to_jopp_object(startup_config const& cfg)
	{
		return std::visit(
			[]<class T>(T const& item) {
				jopp::object ret;
				ret.insert("operational_mode", operational_mode_info<T>::name);
				ret.insert("parameters", to_jopp_object(item));
				return ret;
			},
			cfg
		);
	}
}

#endif