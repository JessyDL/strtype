#pragma once
#include <array>
#include <string_view>
#include <type_traits>

namespace strenum
{
	namespace details
	{
		template <size_t N>
		struct fixed_string;

		template <typename T>
		struct is_fixed_string : std::false_type
		{};
		template <size_t N>
		struct is_fixed_string<fixed_string<N>> : std::true_type
		{};

		template <typename T>
		concept IsFixedString = is_fixed_string<std::remove_cvref_t<T>>::value;

		template <size_t N>
		struct fixed_string
		{
			char buf[N + 1] {};
			consteval fixed_string(char const* s)
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
			}
			auto operator<=>(const fixed_string&) const = default;

			constexpr char operator[](size_t index) const noexcept { return buf[index]; }

			constexpr operator std::string_view() const noexcept { return std::string_view {buf, N}; }
			constexpr operator char const*() const { return buf; }

			constexpr size_t size() const noexcept { return N; }

			template <size_t start, size_t end>
			consteval fixed_string<end - start> substr() const noexcept
			{
				static_assert(start <= end);
				static_assert(end <= N + 1);
				return fixed_string<end - start> {&buf[start]};
			}
		};
		template <unsigned N>
		fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

		consteval auto to_underlying(auto value) { return static_cast<std::underlying_type_t<decltype(value)>>(value); }
		template <typename T>
		concept EnumHasKnownBegin = requires()
		{
			T::_BEGIN;
		};

		template <typename T>
		concept EnumHasKnownEnd = requires()
		{
			T::_END;
		};

		template <EnumHasKnownBegin T>
		consteval auto enum_start() -> std::underlying_type_t<T>
		{
			return to_underlying(T::_BEGIN);
		}

		template <EnumHasKnownEnd T>
		consteval auto enum_end() -> std::underlying_type_t<T>
		{
			return to_underlying(T::_END) + 1;
		}

		template <details::fixed_string signature>
		consteval auto scan_signature_range() -> std::pair<size_t, size_t>
		{
			if(signature.size() == 0) return {};
			size_t depth {0};
			const bool is_funcsig = signature[signature.size() - 1] != ']';
			if(is_funcsig)
			{
				size_t end {0};
				for(auto i = 0; i != signature.size(); ++i)
				{
					auto index = (signature.size() - 1) - i;
					if(signature[index] == '>')
					{
						if(depth == 0)
						{
							end = index;
						}
						++depth;
					}
					else if(signature[index] == '<')
					{
						--depth;
						if(depth == 0)
						{
							return {index + 1, end};
						}
					}
					else if(index > 0 && depth == 1 && signature[index] == ':' && signature[index - 1] == ':')
					{
						return {index + 1, end};
					}
				}
			}
			else
			{
				for(auto i = 0; i != signature.size(); ++i)
				{
					auto index = (signature.size() - 1) - i;
					if(((index > 0 && signature[index] == ':' && signature[index - 1] == ':') ||
						(index + 1 < signature.size() && signature[index] == ' ' && signature[index + 1] == '(')))
					{
						return {index + 1, signature.size() - 1};
					}
				}
			}
			throw std::exception();	   // we couldn't find the start of the signature
		}

		template <details::fixed_string Str>
		consteval auto store_str()
		{
			return &Str.buf[0];
		}

		template <auto v>
		consteval auto stringify_value()
		{
#if defined(__FUNCSIG__)
			constexpr auto signature = details::fixed_string {__FUNCSIG__};
			constexpr auto pair		 = scan_signature_range<__FUNCSIG__>();
#elif defined(__GNUG__)
			constexpr auto signature = details::fixed_string {__PRETTY_FUNCTION__};
			constexpr auto pair		 = scan_signature_range<__PRETTY_FUNCTION__>();
#else
	#error Either __FUNCSIG__ (MSVC) or __PRETTY_FUNCTION__ (GCC/CLang) required
#endif
			constexpr auto substr = signature.template substr<pair.first, pair.second>();
			return store_str<substr>();
		}

		template <typename T, auto V>
		consteval bool is_known_enum_value()
		{
			return stringify_value<T {V}>()[0] != '(';
		}

		template <typename T, auto offset, size_t size, size_t... Indices>
		consteval auto stringify(std::index_sequence<Indices...>)
		{
			std::array<std::string_view, size> storage {};
			size_t index = 0;
			(
			  [&index, &storage]() mutable {
				  if(is_known_enum_value<T, Indices>()) storage[index++] = stringify_value<T {Indices}>();
			  }(),
			  ...);
			return storage;
		}

		template <typename T, auto offset, size_t... Indices>
		consteval auto scan_size(std::index_sequence<Indices...>)
		{
			size_t count = 0;
			([&count]() mutable { count += is_known_enum_value<T, Indices>() ? 1 : 0; }(), ...);
			return count;
		}

		template <typename T, std::underlying_type_t<T> Begin, std::underlying_type_t<T> End>
		requires(std::is_enum_v<T>&& End >= Begin) consteval auto stringify()
		{
			constexpr auto Size = details::scan_size<T, Begin>(std::make_index_sequence<End - Begin>());

			return details::stringify<T, Begin, Size>(std::make_index_sequence<End - Begin>());
		}
	}	 // namespace details

	template <typename T, auto Begin = details::enum_start<T>(), auto End = details::enum_end<T>()>
	requires(std::is_enum_v<T>) consteval auto stringify()
	{
		if constexpr(std::is_same_v<decltype(Begin), T> && std::is_same_v<decltype(End), T>)
		{
			return details::stringify<T, details::to_underlying(Begin), details::to_underlying(End) + 1>();
		}
		else if constexpr(std::is_same_v<decltype(Begin), T> &&
						  std::is_same_v<decltype(End), std::underlying_type_t<T>>)
		{
			return details::stringify<T, details::to_underlying(Begin), End>();
		}
		else if constexpr(std::is_same_v<decltype(Begin), std::underlying_type_t<T>> &&
						  std::is_same_v<decltype(End), T>)
		{
			return details::stringify<T, Begin, details::to_underlying(End) + 1>();
		}
		else if constexpr(std::is_same_v<decltype(Begin), std::underlying_type_t<T>> &&
						  std::is_same_v<decltype(End), std::underlying_type_t<T>>)
		{
			return details::stringify<T, Begin, End>();
		}
		throw std::exception();	   // The values of Begin/End are required to be of the
								   // same type as the enum, or by equivalent to the
								   // underlying type
	}
}	 // namespace strenum