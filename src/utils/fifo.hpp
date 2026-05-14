#ifndef PIPE_UTILS_FIFO_HPP
#define PIPE_UTILS_FIFO_HPP

#include "./scope_handling.hpp"

#include <vector>
#include <type_traits>
#include <cstddef>
#include <cstdio>

namespace Pipe::utils
{
	template<class T, class Allocator = std::allocator<T>>
	class fifo
	{
	public:
		fifo() = default;

		explicit fifo(Allocator const& allocator):
			m_buffer{allocator}
		{}

#if __GNUC__ == 15
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		template<class U>
		requires(std::is_convertible_v<std::remove_cvref_t<U>, T>)
		void push(U&& value)
		{ m_buffer.push_back(std::forward<U>(value)); }
#if __GNUC__ == 15
#pragma GCC diagnostic pop
#endif

		template<class Callback>
		void drain_until_blocked_or_empty(Callback cb) __restrict__
		{
			auto read_offset = m_read_offset;
			auto const elem_count = std::size(m_buffer);
			at_scope_exit _{
				[&](){
					if(read_offset >= elem_count/2)
					{
						m_buffer.erase(std::begin(m_buffer), std::begin(m_buffer) + read_offset);
						read_offset = 0;
					}
					m_read_offset = read_offset;
				}
			};

			while(read_offset != elem_count && cb(m_buffer[read_offset]))
			{ ++read_offset; }
		}

		[[nodiscard]] bool empty() const noexcept
		{ return std::size(m_buffer) == m_read_offset; }

		[[nodiscard]] size_t size() const noexcept
		{ return std::size(m_buffer) - m_read_offset; }

	private:
		size_t m_read_offset{0};
		std::vector<T, Allocator> m_buffer;
	};
}

#endif