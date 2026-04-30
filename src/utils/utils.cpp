//@	{"target":{"name": "utils.o"}}

#include "./utils.hpp"

#include <random>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

std::string Pipe::utils::random_printable_ascii_string(size_t n)
{
	std::uniform_int_distribution<char> char_source{33, 126};
	std::random_device rng{"/dev/urandom"};
	std::string ret;
	ret.reserve(n);
	for(size_t k = 0; k != n; ++k)
	{ ret.push_back(char_source(rng)); }
	return ret;
}

[[gnu::cold]] [[noreturn]] void Pipe::utils::log_and_terminate(std::string_view message) noexcept
{
	::signal(SIGPIPE, SIG_DFL);
	{
		auto const flags = ::fcntl(STDOUT_FILENO, F_GETFL) & (~O_NONBLOCK);
		::fcntl(STDOUT_FILENO, F_SETFL, flags);
	}

	write_buffer buff{
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

[[gnu::cold]] [[noreturn]] void Pipe::utils::log_with_errno_and_terminate(std::string_view message, int) noexcept
{
	log_and_terminate(message);
}