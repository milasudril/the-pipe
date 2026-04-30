//@	{"target":{"name":"log.o"}}

#include "./log.hpp"
#include "src/utils/utils.hpp"

#include <mutex>
#include <utility>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

namespace
{
	constinit Pipe::log::configuration log_cfg;
	constinit std::mutex log_mutex;
};

Pipe::log::configuration Pipe::log::configure(configuration const& cfg) noexcept
{
	std::lock_guard lock{log_mutex};
	return std::exchange(log_cfg, cfg);
}

void Pipe::log::write_message(enum item::level level, std::string&& message)
{
	std::lock_guard lock{log_mutex};
	write_message(
		item{
			.when = log_cfg.timestamp_generator.now(),
			.level = level,
			.message = std::move(message)
		},
		log_cfg.writer
	);
}


[[gnu::cold]] [[noreturn]] void Pipe::log::terminate_with_message(std::string_view message) noexcept
{
	::signal(SIGPIPE, SIG_DFL);
	{
		auto const flags = ::fcntl(STDOUT_FILENO, F_GETFL) & (~O_NONBLOCK);
		::fcntl(STDOUT_FILENO, F_SETFL, flags);
	}

	utils::write_buffer buff{
		[](std::span<char const> data_to_write){
			auto _ = ::write(STDOUT_FILENO, std::data(data_to_write), std::size(data_to_write));
		}
	};

	buff.putchar('\0');
	for(auto item : message)
	{
		buff.putchar(item);
	}

	buff.flush();

	abort();
}

