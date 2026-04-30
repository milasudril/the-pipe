//@	{"dependencies_extra":[{"ref": "./utils.o", "rel": "implementation"}]}

#ifndef PIPE_UTILS_HPP
#define PIPE_UTILS_HPP

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
#include <concepts>
#include <span>
#include <format>
#include <type_traits>
#include <array>
#include <utility>

/**
 * \brief Contains various utility functions
 */
namespace Pipe::utils
{
	/**
	 * \brief Generates a random string of printable non-white-space ASCII characters
	 */
	std::string random_printable_ascii_string(size_t n);

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
		{ sort_and_remove_duplicates(); }

		template<class Iter, class ValueConverter>
		explicit flat_set(Iter begin, Iter end, ValueConverter conv)
		{
			std::transform(
				begin,
				end,
				std::back_inserter(m_values),
				std::forward<ValueConverter>(conv)
			);
			sort_and_remove_duplicates();
		}

		operator span() const
		{ return span{std::data(m_values), std::data(m_values) + std::size(m_values)}; }

	private:
		void sort_and_remove_duplicates()
		{
			std::ranges::sort(m_values);
			auto const res = std::ranges::unique(m_values);
			m_values.erase(res.end(), std::end(m_values));
		}
		std::vector<T> m_values;
	};

	template<class Iter>
	flat_set(Iter, Iter) -> flat_set<typename std::iterator_traits<Iter>::value_type>;

	template<class Iter, class ValueConverter>
	flat_set(Iter begin, Iter end, ValueConverter conv) ->
		flat_set<std::invoke_result_t<ValueConverter, typename std::iterator_traits<Iter>::value_type>>;

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
		{
			func(boundaries, args...);
			return;
		}

		auto i = std::begin(split_points);
		auto index_start = boundaries.start_at;
		if(*i == boundaries.start_at)
		{
			++index_start;
			++i;
		}

		while(i != std::end(split_points))
		{
			auto index_stop = *i;
			if(index_start != index_stop)
			{ func(inclusive_integral_range{index_start, index_stop - 1}, args...); }
			index_start = index_stop + 1;
			++i;
		}

		{
			auto index_stop = boundaries.stop_at;
			if(index_start != index_stop && boundaries.stop_at != split_points.back())
			{ func(inclusive_integral_range{index_start, index_stop}, args...);}
		}
	}

	template<size_t FieldCount>
	struct struct_info
	{
		size_t total_size;
		std::array<size_t, FieldCount> offsets;
	};

	struct struct_field_info
	{
		size_t size;
		size_t alignment;
	};

	template<size_t FieldCount>
	constexpr struct_info<FieldCount> compute_struct_info(
		std::array<struct_field_info, FieldCount> const& fields
	)
	{
		struct_info<FieldCount> ret{};

		size_t current_offset = 0;
		size_t max_alignment = 1;
		for(size_t k = 0; k != FieldCount; ++k)
		{
			max_alignment = std::max(max_alignment, fields[k].alignment);
			current_offset = (
				current_offset/fields[k].alignment + (current_offset % fields[k].alignment != 0)
			)*fields[k].alignment;

			ret.offsets[k] = current_offset;
			current_offset += fields[k].size;
		}

		ret.total_size = (
			current_offset/max_alignment + (current_offset % max_alignment != 0)
		)*max_alignment;
		return ret;
	}

	template<class T>
	struct array_size;

	template<class T, size_t N>
	struct array_size<T[N]>
	{
		static constexpr size_t value = N;
	};

	template<class T>
	inline constexpr auto array_size_v = array_size<T>::value;

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

		void puts(std::string_view str)
		{
			while(!str.empty())
			{
				auto const num_bytes_to_copy = std::min(
					std::size(m_data) - m_write_offest,
					std::size(str)
				);
				if(num_bytes_to_copy == 0)
				{ flush(); }
				else
				{ std::copy_n(std::begin(str), num_bytes_to_copy, std::begin(m_data) + m_write_offest); }
				str = std::string_view{std::begin(str) + num_bytes_to_copy, std::end(str)};
			}
		};

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

	[[gnu::cold]] [[noreturn]] void log_and_terminate(std::string_view message) noexcept;

	[[gnu::cold]] [[noreturn]] void log_with_errno_and_terminate(std::string_view message, int err) noexcept;
};

#endif
