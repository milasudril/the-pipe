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
	/**
	 * \brief Type trait used to serialize/deserialize a host_address
	 */
	template<class T>
	struct host_address_type_info
	{};

	/**
	 * \brief The type of socket to use for the client_ctl protocol
	 */
	using socket_fd_ref = os_services::ipc::connected_socket_ref<SOCK_STREAM, sockaddr_un>;

	/**
	 * \brief Specialization of host_address_type_info for socket_fd_ref
	 */
	template<>
	struct host_address_type_info<socket_fd_ref>
	{
		/**
		 * \brief If host address type is set to socket_fd, a file descriptor referring to a socket
		 *        will be provided by host_address
		 */
		static constexpr const char* name = "socket_fd";
	};

	/**
	 * \brief Convers a socket_fd_ref to jopp::number
	 */
	inline jopp::number to_jopp_value(socket_fd_ref value)
	{
		return static_cast<jopp::number>(value.native_handle());
	}

	/**
	 * \brief The possible ways of specifying a host_address
	 */
	using host_address = std::variant<socket_fd_ref>;

	/**
	 * \brief Converts a host_address to a jopp::object
	 */
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

	/**
	 * \brief Type trait used to serialize/deserialize an operational_mode
	 */
	template<class T>
	struct operational_mode_info
	{};

	/**
	 * \brief Specifies information about the host to connect to
	 */
	struct host_info
	{
		host_address address;
	};

	/**
	 * \brief Specialization of operational_mode_info for host_info
	 */
	template<>
	struct operational_mode_info<host_info>
	{
		/**
		 * \brief If operational_mode is connected_to_host, the client should run connected to a host
		 */
		static constexpr const char* name = "connected_to_host";
	};

	/**
	 * \brief Converts a host_info to a jopp::object
	 */
	inline jopp::object to_jopp_object(host_info const& object)
	{
		jopp::object ret;
		ret.insert("address", to_jopp_object(object.address));
		return ret;
	}

	/**
	 * \brief The type used to identify a port
	 */
	using port_name = std::string;

	/**
	 * \brief Specifies which input file to use for different input ports
	 */
	using file_input_port_map = std::map<std::filesystem::path, std::vector<port_name>>;

	/**
	 * \brief Converts a file_input_port_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(file_input_port_map const& object)
	{
		jopp::object ret;
		for(auto const& item: object)
		{
			jopp::array ports;
			for(auto const& port: item.second)
			{ ports.push_back(port); }

			ret.insert(item.first.string(), std::move(ports));
		}
		return ret;
	}

	/**
	 * \brief Specifies which output files to use for a specific port
	 */
	using output_port_file_map = std::map<port_name, std::vector<std::filesystem::path>>;

	/**
	 * \brief Converts a output_port_file_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(output_port_file_map const& object)
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

	/**
	 * \brief Describes a configuration to use when the client is running in standalone mode
	 */
	struct local_config
	{
		file_input_port_map inputs;
		output_port_file_map outputs;
	};

	/**
	 * \brief Specialization of operational_mode_info for local_config
	 */
	template<>
	struct operational_mode_info<local_config>
	{
		/**
		 * \brief If operational_mode is standalone, the client should run without a host
		 */
		static constexpr const char* name = "standalone";
	};

	/**
	 * \brief Converts a local_config to a jopp::obejct
	 */
	inline jopp::object to_jopp_object(local_config const& cfg)
	{
		jopp::object ret;
		ret.insert("inputs", to_jopp_object(cfg.inputs));
		ret.insert("outputs", to_jopp_object(cfg.outputs));
		return ret;
	}

	/**
	 * \brief The possible startup configurations
	 */
	using startup_config = std::variant<host_info, local_config>;

	/**
	 * \brief Converts a startup_config
	 */
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