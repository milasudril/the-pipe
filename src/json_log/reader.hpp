//@	{"dependencies_extra": [{"ref": "./reader.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_LOG_READER_HPP
#define PIPE_JSON_LOG_READER_HPP

#include "src/log/log.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event.hpp"

#include <jopp/types.hpp>
#include <jopp/parser.hpp>
#include <memory>

namespace Pipe::json_log
{
	class reader
	{
	public:
		explicit reader(size_t buffer_size):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<char[]>(buffer_size)}
		{}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::io::input_file_descriptor_ref fd
		);

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_input_buffer;

		struct state
		{
			state():parser{container}{}

			jopp::container container;
			jopp::parser parser;
		};

		std::unique_ptr<state> m_state;
	};
}

#endif