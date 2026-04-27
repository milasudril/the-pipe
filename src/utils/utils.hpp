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
#include <type_traits>
#include <variant>
#include <array>
#include <utility>
#include <stdexcept>

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

	template<class Callable>
	class at_scope_exit
	{
	public:
		explicit at_scope_exit(Callable&& func):
			m_func{std::move(func)}
		{}

		at_scope_exit(at_scope_exit const&) = delete;
		at_scope_exit(at_scope_exit&&) = delete;
		at_scope_exit& operator=(at_scope_exit const&) = delete;
		at_scope_exit& operator=(at_scope_exit&&) = delete;

		~at_scope_exit()
		{ m_func(); }

	private:
		Callable m_func;
	};

	template<class Callable>
	class maybe_at_scope_exit
	{
	public:
		explicit maybe_at_scope_exit(Callable&& func):
			m_func{std::move(func)}
		{}

		maybe_at_scope_exit(maybe_at_scope_exit const&) = delete;
		maybe_at_scope_exit(maybe_at_scope_exit&&) = delete;
		maybe_at_scope_exit& operator=(maybe_at_scope_exit const&) = delete;
		maybe_at_scope_exit& operator=(maybe_at_scope_exit&&) = delete;

		~maybe_at_scope_exit()
		{
			if(m_func.has_value())
			{ (*m_func)();  }
		}

		void reset()
		{ m_func.reset(); }

	private:
		std::optional<Callable> m_func;
	};

	template<class... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

	template<class T>
	struct array_size;

	template<class T, size_t N>
	struct array_size<T[N]>
	{
		static constexpr size_t value = N;
	};

	template<class T>
	inline constexpr auto array_size_v = array_size<T>::value;

	template<class VariantType, class... TypesToPush>
	struct variant_push_front
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			return std::type_identity<
				std::variant<
					TypesToPush...,
					std::variant_alternative_t<I, VariantType>...
				>
			>{};
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<std::variant_size_v<VariantType>>{})
		)::type;
	};

	template<class VariantType, class... TypesToPush>
	using variant_push_front_t = variant_push_front<VariantType,TypesToPush...>::type;

	template<class VariantType, template<class> class Wrapper>
	struct wrap_variant_element
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			return std::type_identity<
				std::variant<
					Wrapper<std::variant_alternative_t<I, VariantType>>...
				>
			>{};
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<std::variant_size_v<VariantType>>{})
		)::type;
	};

	template<class VariantType, template<class> class Wrapper>
	using wrap_variant_element_t = wrap_variant_element<VariantType,Wrapper>::type;

	template<class Variant, class... CtorArgs>
	Variant make_variant(std::size_t alternative_index, CtorArgs&&... args)
	{
		static constexpr auto factories = []<std::size_t... Is>(std::index_sequence<Is...>){
			return std::array{
				+[](CtorArgs&&... args){
					return Variant{std::in_place_index_t<Is>{}, std::forward<CtorArgs>(args)...};
				}...
			};
		}(std::make_index_sequence<std::variant_size_v<Variant>>{});

		if(alternative_index >= std::size(factories))
		{ throw std::runtime_error{"Invalid type-id"}; }

		return factories[alternative_index](std::forward<CtorArgs>(args)...);
	}

	template <class T, class Variant>
	struct variant_index;

	template<class T>
	struct variant_index_tag{};

	template <class T, class... Types>
	struct variant_index<T, std::variant<Types...>>
	{

		static constexpr size_t value =
			std::variant<variant_index_tag<Types>...>{variant_index_tag<T>{}}.index();
	};

	template<class T, class... Types>
	constexpr auto variant_index_v = variant_index<T, Types...>::value;

	[[gnu::cold]] [[noreturn]] void log_and_terminate(std::string_view message) noexcept;

	[[gnu::cold]] [[noreturn]] void log_with_errno_and_terminate(std::string_view message, int err) noexcept;
};

template<class T>
concept bitmask_enum =
	std::is_enum_v<T> &&
	requires(T e)
{
	{enable_bitmask_operators(e)};
};

template<class T>
requires(std::is_enum_v<T>)
constexpr auto to_underlying(T value)
{
	using underlying = std::underlying_type_t<T>;
	return static_cast<underlying>(value);
}

template<bitmask_enum T>
constexpr T operator~(T value)
{ return static_cast<T>(~to_underlying(value)); }

template<bitmask_enum T>
constexpr T operator|(T a, T b)
{ return static_cast<T>(to_underlying(a) | to_underlying(b)); }

template<bitmask_enum T>
constexpr T operator&(T a, T b)
{ return static_cast<T>(to_underlying(a) & to_underlying(b)); }

template<bitmask_enum T>
constexpr T operator^(T a, T b)
{ return static_cast<T>(to_underlying(a) ^ to_underlying(b)); }

template<bitmask_enum T>
constexpr T& operator|=(T& a, T b)
{ return a = a | b; }

template<bitmask_enum T>
constexpr T& operator&=(T& a, T b)
{ return a = a & b; }

template<bitmask_enum T>
constexpr T& operator^=(T& a, T b)
{ return a = a ^ b; }

template<bitmask_enum T>
constexpr auto is_set(T a, T value)
{ return static_cast<bool>(a & value); }

#endif