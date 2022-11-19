#pragma once
#include <array>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
	#define STRENUM_MSVC 1
	#define STRENUM_SIG __FUNCSIG__
#elif defined(__GNUG__)
	#define STRENUM_GNUG 1
	#define STRENUM_SIG __PRETTY_FUNCTION__
#else
	#error Either __FUNCSIG__ (MSVC) or __PRETTY_FUNCTION__ (GCC/CLang) required
#endif

#if !defined(STRENUM_MAX_SEARCH_SIZE)
	#define STRENUM_MAX_SEARCH_SIZE 1024
#endif

namespace strenum
{
	namespace details
	{
#pragma region fixed_string
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
			static constexpr auto SIZE = N;
			char buf[N + 1] {};
			consteval fixed_string(char const* s)
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
			}
			auto operator<=>(const fixed_string&) const = default;

			constexpr char operator[](size_t index) const noexcept { return buf[index]; }

			constexpr operator std::string_view() const noexcept { return std::string_view {buf, N}; }
			constexpr operator char const*() const { return buf; }

			constexpr auto size() const noexcept -> size_t { return N; }
			constexpr auto empty() const noexcept -> bool { return size() == 0; }

			template <size_t start, size_t end>
			consteval auto substr() const noexcept -> fixed_string<end - start>
			{
				static_assert(start <= end);
				static_assert(end <= N + 1);
				return fixed_string<end - start> {&buf[start]};
			}
		};
		template <unsigned N>
		fixed_string(char const (&)[N]) -> fixed_string<N - 1>;
#pragma endregion fixed_string
#pragma region helpers
		consteval auto to_underlying(auto value) { return static_cast<std::underlying_type_t<decltype(value)>>(value); }

#if !defined(__cpp_lib_is_scoped_enum)
		// taken from cppreference: https://en.cppreference.com/w/cpp/types/is_scoped_enum
		namespace
		{	 // avoid ODR-violation
			template <class T>
			auto test_sizable(int) -> decltype(sizeof(T), std::true_type {});
			template <class>
			auto test_sizable(...) -> std::false_type;

			template <class T>
			auto test_nonconvertible_to_int(int)
			  -> decltype(static_cast<std::false_type (*)(int)>(nullptr)(std::declval<T>()));
			template <class>
			auto test_nonconvertible_to_int(...) -> std::true_type;

			template <class T>
			constexpr bool is_scoped_enum_impl =
			  std::conjunction_v<decltype(test_sizable<T>(0)), decltype(test_nonconvertible_to_int<T>(0))>;
		}	 // namespace

		template <class>
		struct is_scoped_enum : std::false_type
		{};

		template <class E>
			requires std::is_enum_v<E>
		struct is_scoped_enum<E> : std::bool_constant<is_scoped_enum_impl<E>>
		{};

		template <typename T>
		static constexpr auto is_scoped_enum_v = is_scoped_enum<T>::value;
#else
		template <typename T>
		static constexpr auto is_scoped_enum_v = std::is_scoped_enum_v<T>;
#endif

		template <typename T>
		concept IsValidStringifyableEnum = is_scoped_enum_v<T> && std::is_integral_v<std::underlying_type_t<T>>;

		template <typename T>
		concept EnumHasKnownBegin = requires() { T::_BEGIN; };

		template <typename T>
		concept EnumHasKnownEnd = requires() { T::_END; };

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

		template <typename T>
		struct enum_start_t
		{};

		template <EnumHasKnownBegin T>
		struct enum_start_t<T>
		{
			static constexpr auto BEGIN = details::enum_start<T>();
		};

		template <typename T>
		struct enum_end_t
		{};

		template <EnumHasKnownEnd T>
		struct enum_end_t<T>
		{
			static constexpr auto END = details::enum_end<T>();
		};

		// figure out if the Value is either an enum value of the given enum type, or equivalent to the underlying
		// value;
		template <typename T, auto Value>
		concept IsEnumValueOrUnderlying = IsValidStringifyableEnum<T> &&
										  (std::is_same_v<decltype(Value), T> ||
										   std::is_same_v<decltype(Value), std::underlying_type_t<T>>);

		template <typename T, auto Value, bool ApplyOffset = false>
			requires(IsEnumValueOrUnderlying<T, Value>)
		consteval auto guarantee_is_underlying_value() -> std::underlying_type_t<T>
		{
			if constexpr(std::is_same_v<T, decltype(Value)>)
				return to_underlying(Value) + std::underlying_type_t<T>(ApplyOffset ? 1 : 0);
			else
				return Value;
		}

		template <auto Offset, auto... Indices>
		consteval auto make_offset_sequence_impl(std::integer_sequence<decltype(Offset), Indices...>)
		{
			return std::integer_sequence<decltype(Offset), Indices + Offset...> {};
		}

		template <auto Begin, auto End>
		consteval auto make_offset_sequence()
		{
			constexpr auto MAXSIZE = End - Begin;
			return make_offset_sequence_impl<Begin>(std::make_integer_sequence<decltype(Begin), MAXSIZE>());
		}

		enum class dummy
		{
		};

		template <typename T>
		struct is_array : std::false_type
		{};

		template <size_t S>
		struct is_array<std::array<std::string_view, S>> : std::true_type
		{};
#pragma endregion helpers
	}	 // namespace details

	namespace details
	{
		template <auto Value>
		consteval auto get_signature()
		{
			return details::fixed_string {STRENUM_SIG};
		}
		template <auto KnownValue>
		consteval auto get_known_offset() -> size_t
		{
			constexpr auto Str = get_signature<KnownValue>();
			if(Str.size() == 0) throw std::exception();
			size_t depth {0};
#if defined(STRENUM_MSVC)
			for(auto i = 5; i < Str.size(); ++i)
			{
				auto index = (Str.size() - 1) - i;
				if(Str[index] == '>')
				{
					++depth;
				}
				else if(Str[index] == '<')
				{
					--depth;
					if(depth == 0)
					{
						throw std::exception();	   // not possible; should always be enum_class::value
					}
				}
				else if(index > 0 && depth == 1 && Str[index] == ':' && Str[index - 1] == ':')
				{
					return index + 1;
				}
				else if(Str[index] == ')')
				{
					throw std::exception();
				}
			}
#elif defined(STRENUM_GNUG)
			for(auto i = 0; i != Str.size(); ++i)
			{
				auto index = (Str.size() - 1) - i;
				if(index > 0 && Str[index] == ':' && Str[index - 1] == ':')
				{
					return index + 1;
				}
				else if(index + 1 < Str.size() && Str[index] == ' ' && Str[index + 1] == '(')
				{
					throw std::exception();
				}
			}
#endif
			throw std::exception();	   // we couldn't find the start of the signature
		}

		template <auto Value, size_t known_offset>
			requires(details::is_scoped_enum_v<decltype(Value)>)
		consteval auto stringify_value_impl()
		{
			constexpr auto full_signature = details::get_signature<Value>();
#if defined(STRENUM_MSVC)
			constexpr auto end_offset = 7;	  // sizeof(">(void)")
#elif defined(STRENUM_GNUG)
			constexpr auto end_offset = 1;	  // sizeof("]")
#endif

			if constexpr(full_signature.size() <= known_offset)
			{
				return details::fixed_string {""};
			}
			else
			{
				if constexpr(full_signature[known_offset - 1] == ':')
				{
					return full_signature.template substr<known_offset, full_signature.size() - end_offset>();
				}
				else
				{
					return details::fixed_string {""};
				}
			}
		}
	}	 // namespace details

	// returns the value of the given enum as a cross platform (MSVC, GCC, and CLang) consistent fixed_string
	template <auto Value>
		requires(details::is_scoped_enum_v<decltype(Value)>)
	consteval auto stringify()
	{
		return details::stringify_value_impl<Value, details::get_known_offset<Value>()>();
	}

	namespace details
	{
		template <typename... Ts>
		struct get_array_pack_size
		{};

		template <typename T, size_t S, typename... Res>
		struct get_array_pack_size<std::array<T, S>, Res...>
		{
			static constexpr auto value = []() {
				if constexpr(sizeof...(Res) > 0)
				{
					return S + get_array_pack_size<Res...>::value;
				}
				else
				{
					return S;
				}
			}();
		};
	}	 // namespace details

	struct sequential_searcher
	{
		template <typename T, auto Begin, auto End, auto GetEnumName>
		consteval auto operator()() const noexcept
		{
			using underlying_t = std::underlying_type_t<T>;
			// we need this workaround for CLang which has a hard limit of 256 expansions for fold expressions
			constexpr underlying_t PACK_SIZE  = 128;
			constexpr underlying_t remainder  = (End - Begin) % PACK_SIZE;
			constexpr underlying_t iterations = (End - Begin - remainder) / PACK_SIZE;

			auto fn = []<underlying_t Offset, underlying_t Count>() {
				auto internal =
				  []<std::underlying_type_t<T>... Indices>(std::integer_sequence<std::underlying_type_t<T>, Indices...>)
				{
					auto fn = [](std::string_view* buffer = nullptr) {
						size_t count = 0;

						// workaround for MSVC related ICE
						// todo report the ICE, and verify for fix later
						auto set_value = []<details::fixed_string Str>(size_t index, auto& buffer) {
							buffer[index] = Str;
						};

						auto iterator_fn =
						  []<auto Index>(auto& set_value, auto& count, std::string_view* buffer)
						{
							constexpr auto enum_value {T {Index}};
							constexpr auto name = GetEnumName.template operator()<enum_value>();
							if(!name.empty())
							{
								if(buffer) set_value.template operator()<name>(count, buffer);
								++count;
							}
						};

						(iterator_fn.template operator()<Indices>(set_value, count, buffer), ...);
						return count;
					};
					constexpr auto size = fn();

					std::array<std::string_view, size> result {};
					fn(result.data());
					return result;
				};

				return internal(details::make_offset_sequence<Offset, Offset + Count>());
			};

			auto fn_iters =
			  [&fn]<std::underlying_type_t<T>... Indices>(std::integer_sequence<std::underlying_type_t<T>, Indices...>)
			{
				auto make_array = []<typename... Ts>(Ts&&... arrays) {
					constexpr auto total_size = details::get_array_pack_size<Ts...>::value;
					std::array<std::string_view, total_size> res {};
					size_t offset {0};
					auto fill = [](auto& dst, const auto& src, size_t& offset) {
						for(const auto& v : src)
						{
							dst[offset++] = v;
						}
					};
					(fill(res, arrays, offset), ...);
					return res;
				};

				return make_array(fn.template operator()<Begin + (Indices * PACK_SIZE), PACK_SIZE>()...,
								  fn.template operator()<Begin + (iterations * PACK_SIZE), remainder>());
			};
			return fn_iters(std::make_integer_sequence<std::underlying_type_t<T>, iterations>());
		}
	};

	struct bitflag_searcher
	{};

	template <details::IsValidStringifyableEnum T>
	struct enum_information : public details::enum_start_t<T>, details::enum_end_t<T>
	{
		using SEARCHER = sequential_searcher;
	};

	namespace details
	{
		template <typename T>
		concept HasMaxSearchSizeOverride = requires() { enum_information<T>::MAX_SEARCH_SIZE; };

		template <typename T>
		consteval auto max_search_size() -> size_t
		{
			if constexpr(HasMaxSearchSizeOverride<T>)
			{
				return enum_information<T>::MAX_SEARCH_SIZE;
			}
			else
			{
				return STRENUM_MAX_SEARCH_SIZE;
			}
		}

		template <typename T, auto Begin, auto End, typename Searcher, size_t KnownOffset>
		consteval auto get_unique_entries()
		{
			constexpr auto MAXSIZE = End - Begin;
			static_assert(
			  MAXSIZE <= max_search_size<T>(),
			  "Up the max search size for this enum. Either prefferably by specializing the 'enum_information<T>' and "
			  "setting MAX_SEARCH_SIZE higher, or by upping the define (which would globally increase it)");

			constexpr auto result = Searcher {}.template operator()<T, Begin, End, []<T value>() {
				return details::stringify_value_impl<value, KnownOffset>();
			}>();
			static_assert(details::is_array<std::remove_cvref_t<decltype(result)>>::value,
						  "the result type should be of `std::array<std::string_view, SIZE>` from the searcher.");
			return result;
		}
	}	 // namespace details

	template <details::IsValidStringifyableEnum T,
			  auto Begin		= enum_information<T>::BEGIN,
			  auto End			= enum_information<T>::END,
			  typename Searcher = typename enum_information<T>::SEARCHER>
	consteval auto stringify()
	{
		constexpr auto begin = details::guarantee_is_underlying_value<T, Begin>();
		constexpr auto end	 = details::guarantee_is_underlying_value<T, End, true>();
		static_assert(begin < end, "The end value should be larger than begin");
		return details::get_unique_entries<T, begin, end, Searcher, details::get_known_offset<T {Begin}>()>();
	}
}	 // namespace strenum

#undef STRENUM_MSVC
#undef STRENUM_GNUG
#undef STRENUM_SIG
