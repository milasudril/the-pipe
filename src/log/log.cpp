//@	{"target":{"name":"log.o"}}

#include "./log.hpp"

#include <mutex>
#include <utility>

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

void Pipe::log::write_message(enum item::severity severity, std::string&& message)
{
	std::lock_guard lock{log_mutex};
	write_message(
		item{
			.when = log_cfg.timestamp_generator.now(),
			.severity = severity,
			.message = std::move(message)
		},
		log_cfg.writer
	);
}