#ifndef PIPE_CLIENT_HPP
#define PIPE_CLIENT_HPP

#include "src/os_services/io/io.hpp"

/**
 * \brief Contains the API published by the client
 */
namespace Pipe::client
{
	/**
	 * \brief Represents the state of a client
	 */
	class state
	{
	public:
		virtual ~state() = default;
		/**
		 * \brief This function should process data, by reading and writing associated file descriptors
		 */
		virtual void process_data() = 0;
	};

	/**
	 * \brief Main entry point that creates a client
	 * \param inputs File descriptors to read from
	 * \param outputs File descriptors to write to
   */
	std::unique_ptr<client_state> create_client(
		std::span<io::input_file_descriptor_ref const> inputs,
		std::span<io::output_file_descriptor_ref const> outputs
	);
};

#endif