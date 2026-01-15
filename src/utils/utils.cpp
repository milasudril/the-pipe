//@	{"target":{"name": "utils.o"}}

#include "./utils.hpp"

#include <random>

std::string prog::utils::random_printable_ascii_string(size_t n)
{
	std::uniform_int_distribution<char> char_source{33, 126};
	std::random_device rng{"/dev/urandom"};
	std::string ret;
	ret.reserve(n);
	for(size_t k = 0; k != n; ++k)
	{ ret.push_back(char_source(rng)); }
	return ret;
}