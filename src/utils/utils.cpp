//@	{"target":{"name": "utils.o"}}

#include "./utils.hpp"

#include <random>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

std::vector<std::byte> Pipe::utils::random_bytes(size_t n)
{
	std::uniform_int_distribution<uint8_t> byte_source{0, 255};
	std::random_device rng{"/dev/urandom"};
	std::vector<std::byte> ret;
	ret.reserve(n);
	for(size_t k = 0; k != n; ++k)
	{ ret.push_back(static_cast<std::byte>(byte_source(rng))); }
	return ret;
}

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

namespace
{
	template<class FlushFunc>
	class write_buffer
	{
	public:
		explicit write_buffer(FlushFunc&& func):
			m_flush{std::move(func)}
		{}

		void putchar(char val)
		{
			if(m_write_offest == std::size(m_data)) [[unlikely]]
			{ flush(); }
			m_data[m_write_offest] = val;
			++m_write_offest;
		}

		void flush()
		{
			m_flush(std::span{std::data(m_data), m_write_offest});
			m_write_offest = 0;
		}

	private:
		std::array<char, 4096> m_data;
		size_t m_write_offest{0};
		FlushFunc m_flush;
	};

	template<class FlushFunc>
	write_buffer(FlushFunc&&) ->write_buffer<FlushFunc>;
};

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