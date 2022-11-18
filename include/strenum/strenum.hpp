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
	enum class pattern
	{
		UNKNOWN,	// will try to figure out if the type is sequential or bitflag, but it's best to set this manually
		SEQUENTIAL,	   // do the enum values come sequentially? i.e. 0, 1, 2, 3, ..., 9, 10 .. (does not need to be
					   // consecutive)
		FLAG,		   // are the values more akin to bit flags? i.e. 1, 2, 4, 8, 16, ..., 512, 1024 ..
	};

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
		template<typename T>
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
#pragma endregion helpers
	}	 // namespace details

	template <details::IsValidStringifyableEnum T>
	struct enum_information : public details::enum_start_t<T>, details::enum_end_t<T>
	{
		static constexpr pattern PATTERN = pattern::UNKNOWN;
	};

	// returns the value of the given enum as a cross platform (MSVC, GCC, and CLang) consistent fixed_string
	template <auto Value>
		requires(details::is_scoped_enum_v<decltype(Value)>)
	consteval auto stringify()
	{
		constexpr auto full_signature = details::fixed_string {STRENUM_SIG};

		constexpr auto range = [&signature = full_signature]() -> std::pair<size_t, size_t> {
			if(signature.size() == 0) return {};
			size_t depth {0};
#if defined(STRENUM_MSVC)
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
#elif defined(STRENUM_GNUG)
			for(auto i = 0; i != signature.size(); ++i)
			{
				auto index = (signature.size() - 1) - i;
				if(((index > 0 && signature[index] == ':' && signature[index - 1] == ':') ||
					(index + 1 < signature.size() && signature[index] == ' ' && signature[index + 1] == '(')))
				{
					return {index + 1, signature.size() - 1};
				}
			}
#endif
			throw std::exception();	   // we couldn't find the start of the signature
		}();
		constexpr auto signature = full_signature.template substr<range.first, range.second>();
		if constexpr(!signature.empty() && signature[0] != '(')
			return signature;
		else
			return details::fixed_string {""};
	}

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

		template <typename T, pattern Pattern, std::underlying_type_t<T> Offset, std::underlying_type_t<T>... Indices>
		consteval auto get_named_entries_count(std::integer_sequence<std::underlying_type_t<T>, Indices...>) -> size_t
			requires(Pattern == pattern::SEQUENTIAL)
		{
			size_t count = 0;
			([&count]() mutable { count += (!stringify<T {Indices + Offset}>().empty()); }(), ...);
			return count;
		}

		template <typename T, auto Begin, auto End, pattern Pattern>
		consteval auto get_unique_entries()
			requires(Pattern != pattern::UNKNOWN)
		{
			constexpr auto MAXSIZE = End - Begin;
			static_assert(
			  MAXSIZE <= max_search_size<T>(),
			  "Up the max search size for this enum. Either prefferably by specializing the 'enum_information<T>' and "
			  "setting MAX_SEARCH_SIZE higher, or by upping the define (which would globally increase it)");

			constexpr auto size = get_named_entries_count<T, Pattern, Begin>(
			  std::make_integer_sequence<std::underlying_type_t<T>, MAXSIZE>());

			if constexpr(Pattern == pattern::SEQUENTIAL)
			{
				return []<size_t Size, std::underlying_type_t<T> Offset, std::underlying_type_t<T>... Indices>(
				  std::integer_sequence<std::underlying_type_t<T>, Indices...>)
				{
					size_t index = 0;
					std::array<std::string_view, Size> result;

					auto set_value = []<details::fixed_string Str>(size_t index, auto& buffer) { buffer[index] = Str; };
					(
					  [&index, &result, set_value]() mutable {
						  constexpr auto name = stringify<T {Indices + Offset}>();
						  if(!name.empty())
						  {
							  set_value.template operator()<name>(index++, result);
						  }
					  }(),
					  ...);
					return result;
				}
				.template operator()<size, Begin>(std::make_integer_sequence<std::underlying_type_t<T>, MAXSIZE>());
			}
		}

		template <typename T, auto Begin, auto End, pattern Pattern>
		consteval auto get_unique_entries()
			requires(Pattern == pattern::UNKNOWN)
		{
			return get_unique_entries<T, Begin, End, pattern::SEQUENTIAL>();
		}
	}	 // namespace details

	template <details::IsValidStringifyableEnum T,
			  auto Begin	  = enum_information<T>::BEGIN,
			  auto End		  = enum_information<T>::END,
			  pattern Pattern = enum_information<T>::PATTERN>
	consteval auto stringify()
	{
		constexpr auto begin = details::guarantee_is_underlying_value<T, Begin>();
		constexpr auto end	 = details::guarantee_is_underlying_value<T, End, true>();
		static_assert(begin < end, "The end value should be larger than begin");
		return details::get_unique_entries<T, begin, end, Pattern>();
	}
}	 // namespace strenum

#undef STRENUM_MSVC
#undef STRENUM_GNUG
#undef STRENUM_SIG
