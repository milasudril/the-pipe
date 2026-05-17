#ifndef PIPE_UTILS_STRING_LITERAL_HPP
#define PIPE_UTILS_STRING_LITERAL_HPP

#include <string_view>

namespace Pipe::utils
{
	class string_literal
	{
		public:
			template<class T>
			requires(std::is_convertible_v<T, std::string_view>)
			consteval string_literal(T&& content):
				m_content{content}
			{}

			constexpr auto operator<=>(string_literal const& other) const = default;

			constexpr auto content() const
			{ return m_content; }

		private:
			std::string_view m_content;
	};
};

#endif