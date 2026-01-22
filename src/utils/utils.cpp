//@	{"target":{"name": "utils.o"}}

#include "./utils.hpp"

#include <random>

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
