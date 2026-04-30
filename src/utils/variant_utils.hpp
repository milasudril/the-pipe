#ifndef PIPE_UTILS_VARIANT_UTILS_HPP
#define PIPE_UTILS_VARIANT_UTILS_HPP

#include <type_traits>
#include <variant>
#include <cstddef>
#include <stdexcept>

namespace Pipe::utils
{
	template<class... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

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
}
#endif
