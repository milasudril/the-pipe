#ifndef PIPE_CLIENT_CTL_STARTUP_CONFIG_HPP
#define PIPE_CLIENT_CTL_STARTUP_CONFIG_HPP

#include "src/os_services/error_handling/system_error.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"

#include <jopp/types.hpp>
#include <variant>
#include <map>
#include <vector>
#include <filesystem>
#include <sys/stat.h>

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
	 * \brief Converts a socket_fd_ref to jopp::number
	 */
	inline jopp::number to_jopp_value(socket_fd_ref value)
	{
		return static_cast<jopp::number>(value.native_handle());
	}

	/**
	 * \brief Converts a jopp::number to a socket_fd_ref
	 */
	inline socket_fd_ref make_socket_fd_ref(jopp::number value)
	{
		if(value < 0.0 || value > 2147483647.0)
		{ throw std::runtime_error{"Invalid socket_fd"};}

		auto const fd_val = static_cast<int>(value);
		if(static_cast<double>(fd_val) != value)
		{ throw std::runtime_error{"Invalid socket_fd"};}

		struct stat statbuf{};
		if(fstat(fd_val, &statbuf) == -1)
		{ throw os_services::error_handling::system_error{"Invalid socket_fd", errno}; }

		if(!S_ISSOCK(statbuf.st_mode))
		{ throw std::runtime_error{"socket_fd is not a socket"}; }

		return socket_fd_ref{fd_val};
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
	 * \brief Converts a jopp::object to a host_address
	 */
	inline host_address make_host_address(jopp::object const& obj)
	{
		auto const& type = obj.get_field_as<jopp::string>("type");
		if(type == host_address_type_info<socket_fd_ref>::name)
		{ return make_socket_fd_ref(obj.get_field_as<jopp::number>("value")); }

		throw std::runtime_error{"The given host address type is not supported"};
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
	 * \brief Converts a jopp::object to a host_info
	 */
	inline host_info make_host_info(jopp::object const& object)
	{
		return host_info{
			.address = make_host_address(object.get_field_as<jopp::object>("address"))
		};
	}

	/**
	 * \brief The type used to identify a port
	 */
	using port_name = std::string;

	/**
	 * \brief Specifies which input file to use for different input ports
	 */
	using input_port_file_map = std::map<port_name, std::filesystem::path>;

	/**
	 * \brief Converts a input_port_file_map to a jopp::object
	 */
	inline jopp::object to_jopp_object(input_port_file_map const& object)
	{
		jopp::object ret;
		for(auto const& item: object)
		{ ret.insert(jopp::string{item.first}, item.second.string()); }
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a input_port_file_map
	 */
	inline input_port_file_map make_input_port_file_map(jopp::object const& object)
	{
		input_port_file_map ret;
		for(auto const& item:object)
		{ ret.insert(std::pair{item.first, item.second.get<jopp::string>()}); }
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

			ret.insert(jopp::string{item.first}, std::move(paths));
		}
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a output_port_file_map
	 */
	inline output_port_file_map make_output_port_file_map(jopp::object const& object)
	{
		output_port_file_map ret;
		for(auto const& item: object)
		{
			std::vector<std::filesystem::path> paths;
			for(auto const& item : item.second.get<jopp::array>())
			{ paths.push_back(item.get<jopp::string>()); }
			ret.insert(std::pair{item.first, std::move(paths)});
		}
		return ret;
	}

	/**
	 * \brief Describes a configuration to use when the client is running in standalone mode
	 */
	struct standalone_config
	{
		input_port_file_map inputs;
		output_port_file_map outputs;
	};

	/**
	 * \brief Specialization of operational_mode_info for standalone_config
	 */
	template<>
	struct operational_mode_info<standalone_config>
	{
		/**
		 * \brief If operational_mode is standalone, the client should run without a host
		 */
		static constexpr const char* name = "standalone";
	};

	/**
	 * \brief Converts a standalone_config to a jopp::obejct
	 */
	inline jopp::object to_jopp_object(standalone_config const& cfg)
	{
		jopp::object ret;
		ret.insert("inputs", to_jopp_object(cfg.inputs));
		ret.insert("outputs", to_jopp_object(cfg.outputs));
		return ret;
	}

	/**
	 * \brief Converts a jopp::object to a standalone_config
	 */
	inline standalone_config make_standalone_config(jopp::object const& obj)
	{
		return standalone_config{
			.inputs = make_input_port_file_map(obj.get_field_as<jopp::object>("inputs")),
			.outputs = make_output_port_file_map(obj.get_field_as<jopp::object>("outputs"))
		};
	}

	/**
	 * \brief The possible startup configurations
	 */
	using startup_config = std::variant<host_info, standalone_config>;

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

	/**
	 * \brief Converts a jopp::object to a startup_config
	 */
	inline startup_config make_startup_config(jopp::object const& object)
	{
		auto const& operational_mode = object.get_field_as<jopp::string>("operational_mode");
		if(operational_mode == "connected_to_host")
		{ return make_host_info(object.get_field_as<jopp::object>("parameters")); }
		else
		if(operational_mode == "standalone")
		{ return make_standalone_config(object.get_field_as<jopp::object>("parameters")); }

		throw std::runtime_error{"The given operational mode is not supported"};
	}
}

#endif