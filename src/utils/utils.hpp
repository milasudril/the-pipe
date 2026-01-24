//@	{"dependencies_extra":[{"ref": "./utils.o", "rel": "implementation"}]}

#ifndef PIPE_UTILS_HPP
#define PIPE_UTILS_HPP

#include <algorithm>
#include <string>
#include <cmath>
#include <vector>
#include <concepts>
#include <span>
#include <format>

/**
 * \brief Contains various utility functions
 */
namespace Pipe::utils
{
	/**
	 * \brief Generates an array of random bytes
	 */
	std::vector<std::byte> random_bytes(size_t n);

	/**
	 * \brief The number of printable ASCII characters (all white-space excluded)
	 */
	constexpr size_t num_printable_ascii_chars = 94;

	/**
	 * \brief Generates a random string of printable non-white-space ASCII characters
	 */
	std::string random_printable_ascii_string(size_t n);

	/**
	 * \brief Computes the number of printable ASCII characters required to represent num_bytes
	 */
	constexpr size_t byte_count_to_printable_ascii_string_length(size_t num_bytes)
	{
		//
		// num_printable_ascii_chars^x = 256^num_bytes
		// x*log2(num_printable_ascii_chars) = num_bytes*log2(256)
		// x = num_bytes*8/log2(num_printable_ascii_chars)

		return static_cast<size_t>(
			std::ceil(
				static_cast<double>(num_bytes)*8.0
					/std::log2(static_cast<double>(num_printable_ascii_chars))
			)
		);
	}

	/**
	 * \brief The number of printable ASCII characters requires for 16 bytes
	 */
	constexpr size_t num_chars_16_bytes = byte_count_to_printable_ascii_string_length(16);

	template<class T>
	concept reference = std::is_reference_v<T>;

	template<class T>
	concept is_refwrapper = requires(T obj)
	{
		{obj.get()} -> reference;
	};

	template<class T>
	concept is_dereferenceable = requires(T obj)
	{
		{*obj};
	};

	template<class T, size_t N>
	consteval void detect_static_array(T (&)[N]){}

	template<class T>
	concept is_c_style_array = std::is_array_v<T> || requires(T obj, size_t x)
	{
		{detect_static_array(obj)};
	};

	/**
	 * \brief A utility function to access the object behind ref
	 */
	template<class T>
	inline constexpr decltype(auto) unwrap(T&& ref)
	{
		if constexpr(is_refwrapper<T>)
		{ return ref.get(); }
		else
		if constexpr(is_dereferenceable<T> && !is_c_style_array<T>)
		{ return *ref; }
		else
		{ return std::forward<T>(ref); }
	}


	template<class T>
	class flat_set
	{
	public:
		class span:std::span<T const>
		{
		public:
			constexpr span() = default;

			using std::span<T const>::begin;
			using std::span<T const>::end;
			using std::span<T const>::operator[];
			using std::span<T const>::empty;
			using std::span<T const>::size;
			using std::span<T const>::front;
			using std::span<T const>::back;
			using iterator = std::span<T const>::iterator;

			constexpr span trim(iterator begin, iterator end) const
			{
				return span{begin, end};
			}

		private:
			constexpr explicit span(T const* begin, T const* end):std::span<T const>{begin, end}{}
			constexpr explicit span(iterator begin, iterator end):std::span<T const>{begin, end}{}

			friend class flat_set;
		};

		template<class Iter>
		explicit flat_set(Iter begin, Iter end):
			m_values{begin, end}
		{ std::ranges::sort(m_values); }

		operator span() const
		{ return span{std::data(m_values), std::data(m_values) + std::size(m_values)}; }

	private:
		std::vector<T> m_values;
	};

	template<class Iter>
	flat_set(Iter, Iter) -> flat_set<typename std::iterator_traits<Iter>::value_type>;

	template<class T>
	using immutable_flat_set = flat_set<T>::span;

	template<std::unsigned_integral T>
	struct inclusive_integral_range
	{
		T start_at;
		T stop_at;

		constexpr bool operator==(inclusive_integral_range const&) const = default;
		constexpr bool operator!=(inclusive_integral_range const&) const = default;
	};

	template<std::unsigned_integral T>
	inline std::string to_string(inclusive_integral_range<T> range)
	{
		return std::format("[{}, {}]", range.start_at, range.stop_at);
	}

	template<class T>
	constexpr auto trim(inclusive_integral_range<T> boundaries, immutable_flat_set<T> vals)
	{
		auto const start_at = std::ranges::find_if(
			vals,
			[start_at = boundaries.start_at](auto const& val) {
				return val >= start_at;
			}
		);

		auto const stop_at = std::find_if(
			start_at,
			std::end(vals),
			[stop_at = boundaries.stop_at](auto const& val) {
				return val > stop_at;
			}
		);

		if(start_at == std::end(vals) || stop_at == std::begin(vals))
		{ return immutable_flat_set<T>{}; }

		return vals.trim(start_at, stop_at);
	}

	/**
	 * \brief Splits the range given by boundaries, at the values given by split_points
	 *
	 * \param boundaries The inclusive start and end points
	 * \param split_points The points to split the range at. The span must represent a strictly
	 *                     monotonic and increasing sequence.
	 * \param func A callable object to call on each detected range
	 * \param args Additional arguments that should be passed to func
	 */
	template<std::unsigned_integral SplitPoint, class Callable, class ... Args>
	constexpr void for_each_disjoint_segment(
		inclusive_integral_range<SplitPoint> boundaries,
		immutable_flat_set<SplitPoint> split_points,
		Callable func,
		Args... args
	)
	{
		split_points = trim(boundaries, split_points);

		if(split_points.empty())
		{ return; }

		auto first = boundaries.start_at;

		for(size_t k = 0; k != std::size(split_points) - 1; ++k)
		{
			auto const index_start = split_points[k] == first? split_points[k] + 1 : first + (k != 0);
			auto const index_end = split_points[k] == first? split_points[k + 1] - 1 : split_points[k] - 1;
			if(index_end == boundaries.stop_at - 1)
			{
				if(index_start <= index_end)
				{ func(inclusive_integral_range{index_start, index_end}, args...); }
				return;
			}
			if(index_start <= index_end)
			{ func(inclusive_integral_range{index_start, index_end}, args...); }
			first = index_end + 1;
		}

		if(boundaries.start_at != split_points.front() && first + 1 <= split_points.back() - 1)
		{ func(inclusive_integral_range{first + 1, split_points.back() - 1}, args...); }

		if(boundaries.stop_at != split_points.back() && split_points.back() + 1 <= boundaries.stop_at)
		{ func(inclusive_integral_range{split_points.back() + 1, boundaries.stop_at}, args...); }
	}
};

#endif