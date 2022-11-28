#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <compare>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
	#define STRTYPE_MSVC 1
	#define STRTYPE_SIG __FUNCSIG__
#elif defined(__GNUG__)
	#define STRTYPE_GNUG 1
	#define STRTYPE_SIG __PRETTY_FUNCTION__
#else
	#error Either __FUNCSIG__ (MSVC) or __PRETTY_FUNCTION__ (GCC/CLang) required
#endif

#if !defined(STRTYPE_MAX_SEARCH_SIZE)
	#define STRTYPE_MAX_SEARCH_SIZE 1024
#endif

namespace strtype
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

			consteval fixed_string(const std::array<char, N>& s)
			{
				for(size_t i = 0; i != N; ++i) buf[i] = s[i];
				buf[N] = '\0';
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

			consteval auto reverse() const noexcept -> fixed_string<SIZE>
			{
				std::array<char, SIZE> copy {};
				constexpr auto halfpoint = (SIZE - (SIZE % 2)) / 2;
				for(size_t i = 0; i != halfpoint; ++i)
				{
					copy[i]			   = buf[SIZE - i - 1];
					copy[SIZE - i - 1] = buf[i];
				}
				if(SIZE % 2 != 0) copy[halfpoint] = buf[halfpoint];
				return fixed_string<SIZE>(copy.data());
			}
		};
		template <unsigned N>
		fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

		template <size_t N>
		fixed_string(const std::array<char, N>) -> fixed_string<N>;
#pragma endregion fixed_string
#pragma region helpers
		constexpr auto to_underlying(auto value) { return static_cast<std::underlying_type_t<decltype(value)>>(value); }

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

		template <auto Offset, typename T, auto... Indices>
		consteval auto make_offset_sequence_impl(std::integer_sequence<T, Indices...>)
		{
			return std::integer_sequence<T, Indices + Offset...> {};
		}

		template <auto Offset, auto Size, typename T = decltype(Offset)>
		consteval auto make_offset_sequence()
		{
			return make_offset_sequence_impl<Offset>(std::make_integer_sequence<T, Size>());
		}

		enum class dummy
		{
		};

		template <typename Y, typename T>
		struct is_array_of_type : std::false_type
		{};

		template <typename T, size_t S>
		struct is_array_of_type<T, std::array<T, S>> : std::true_type
		{};

		template <typename EnumType, typename T>
		struct is_required_return_type : std::false_type
		{};

		template <typename EnumType, typename T, typename Y>
			requires(is_array_of_type<std::string_view, T>::value && is_array_of_type<EnumType, Y>::value)
		struct is_required_return_type<EnumType, std::pair<T, Y>> : std::true_type
		{};

		template <typename EnumType, typename T>
		concept IsRequiredReturnType = is_required_return_type<EnumType, T>::value;

#pragma endregion helpers
#pragma region compile_time_map
		inline constexpr std::uint32_t fnv1a_32(std::string_view value)
		{
			std::uint32_t seed {2166136261u};
			for(auto c : value)
			{
				seed ^= c * 16777619u;
			}
			return seed;
		}

		template <size_t Size>
		inline constexpr std::uint32_t fnv1a_32(const std::array<std::byte, Size>& value)
		{
			std::uint32_t seed {2166136261u};
			for(auto c : value)
			{
				seed ^= std::uint8_t(c) * 16777619u;
			}
			return seed;
		}

		template <typename T>
			requires(std::is_integral_v<T>)
		constexpr auto to_byte_array(const T& value) -> std::array<std::byte, sizeof(T) / sizeof(std::byte)>
		{
			constexpr auto S = sizeof(T) / sizeof(std::byte);
			size_t offset	 = (S - 1) * 8;
			std::array<std::byte, S> result {};
			for(auto i = 1; i < S; ++i)
			{
				auto index	  = S - (i);
				result[index] = std::byte((value >> offset) & 255);
				offset -= 8;
			}
			result[0] = std::byte(value & 255);
			return result;
		}

		template <typename T>
			requires(std::is_integral_v<T>)
		inline constexpr std::uint32_t fnv1a_32(const T& value)
		{
			return fnv1a_32(to_byte_array<T>(value));
		}

		template <typename T, size_t Size>
		struct ct_bst
		{
		  public:
			using string_hash_pair_t = std::pair<std::uint32_t, size_t>;				// hash + index
			using value_hash_pair_t	 = std::pair<std::underlying_type_t<T>, size_t>;	// hash + index
			using value_pair_t		 = std::pair<std::string_view, T>;
			consteval ct_bst(const auto& strs, const auto& values)
			{
				// iterate over all values, turn them into hashed values, and then sort them based on hashes.
				for(size_t i = 0; i < values.size(); ++i)
				{
					m_Data[i]		= value_pair_t {strs[i], values[i]};
					m_StringHash[i] = string_hash_pair_t {fnv1a_32(m_Data[i].first), i};
					m_ValueHash[i]	= value_hash_pair_t {to_underlying<T>(m_Data[i].second), i};
				}
				std::sort(std::begin(m_StringHash), std::end(m_StringHash), [](const auto& lhs, const auto& rhs) {
					return lhs.first < rhs.first;
				});
				std::sort(std::begin(m_ValueHash), std::end(m_ValueHash), [](const auto& lhs, const auto& rhs) {
					return lhs.first < rhs.first;
				});

				if(std::adjacent_find(
					 std::begin(m_StringHash), std::end(m_StringHash), [](const auto& lhs, const auto& rhs) {
						 return lhs.first == rhs.first;
					 }) == std::end(m_StringHash))
				{
					m_PerfectHash = true;
				}

				if(std::adjacent_find(
					 std::begin(m_ValueHash), std::end(m_ValueHash), [](const auto& lhs, const auto& rhs) {
						 return lhs.first == rhs.first;
					 }) != std::end(m_ValueHash))
				{
					throw std::exception();
				}
			}

			constexpr auto operator[](std::string_view value) const -> T
			{
				// todo: can turn this into a compile time constant
				if(m_PerfectHash)
				{
					auto it = std::find_if(std::begin(m_StringHash),
										   std::end(m_StringHash),
										   [hash = fnv1a_32(value)](const auto& value) { return hash == value.first; });

					if(it != std::end(m_StringHash) && m_Data[it->second].first == value)
						return m_Data[it->second].second;
				}
				else
				{
					const string_hash_pair_t hash {fnv1a_32(value), {}};
					auto [begin, end] = std::equal_range(
					  std::begin(m_StringHash), std::end(m_StringHash), hash, [](const auto& lhs, const auto& rhs) {
						  return lhs.first < rhs.first;
					  });

					for(auto it = begin; it != end; it = std::next(it))
					{
						if(m_Data[it->second].first == value) return m_Data[it->second].second;
					}
				}
				throw std::exception(/* missing value */);
			}

			constexpr auto operator[](T value) const -> std::string_view
			{
				auto it =
				  std::find_if(std::begin(m_ValueHash),
							   std::end(m_ValueHash),
							   [hash = to_underlying<T>(value)](const auto& value) { return hash == value.first; });

				if(it != std::end(m_ValueHash) && m_Data[it->second].second == value) return m_Data[it->second].first;

				throw std::exception(/* missing value */);
			}

			constexpr auto size() const noexcept -> size_t { return Size; }
			constexpr auto begin() const noexcept { return std::begin(m_Data); }
			constexpr auto cbegin() const noexcept { return std::begin(m_Data); }
			constexpr auto end() const noexcept { return std::end(m_Data); }
			constexpr auto cend() const noexcept { return std::cend(m_Data); }

			constexpr auto string_at_index(size_t i) const noexcept -> const std::string_view&
			{
				return m_Data[i].first;
			}
			constexpr auto value_at_index(size_t i) const noexcept -> const T& { return m_Data[i].second; }

		  private:
			std::array<string_hash_pair_t, Size> m_StringHash {};
			std::array<value_hash_pair_t, Size> m_ValueHash {};
			std::array<value_pair_t, Size> m_Data {};
			bool m_PerfectHash {false};
		};

#pragma endregion compile_time_map
	}	 // namespace details

	namespace details
	{
		template <auto Value>
		consteval auto get_signature()
		{
			return details::fixed_string {STRTYPE_SIG};
		}

		template <typename T>
		consteval auto get_signature()
		{
			return details::fixed_string {STRTYPE_SIG};
		}

		struct typename_signature_offset
		{
			static constexpr size_t value = []() constexpr -> size_t {
				constexpr auto full_signature = get_signature<void>();
				size_t depth {0};
#if defined(STRTYPE_MSVC)
				for(auto i = 0; i < full_signature.size(); ++i)
				{
					size_t index = full_signature.size() - 1 - i;
					if(full_signature[index] == '>')
					{
						++depth;
					}
					else if(full_signature[index] == '<')
					{
						if(depth == 0)
						{
							throw std::exception();
						}
						--depth;
						if(depth == 0)
						{
							return index + 1;
						}
					}
				}
#elif defined(STRTYPE_GNUG)
				for(auto i = 0; i != full_signature.size(); ++i)
				{
					auto index = (full_signature.size() - 1) - i;
					if(full_signature[index] == '=')
					{
						return index + 2;
					}
				}
#endif
				throw std::exception();
			}();

			template <fixed_string Str>
			static constexpr auto transform()
			{
#if defined(STRTYPE_MSVC)
				constexpr auto end_offset = 7;	  // sizeof(">(void)")
#elif defined(STRTYPE_GNUG)
				constexpr auto end_offset = 1;	  // sizeof("]")
#endif
				return Str.template substr<value, Str.size() - end_offset>();
			}
		};

		constexpr auto get_scope_impl(std::string_view str, char* buffer = nullptr) -> size_t
		{
			size_t total_size {0};
#if defined(STRTYPE_MSVC)
			constexpr std::string_view STRUCT {"struct "};
			constexpr std::string_view CLASS {"class "};
#endif
			for(size_t i = 0; i < str.size(); ++i)
			{
#if defined(STRTYPE_MSVC)
				if(str.substr(i, STRUCT.size()) == STRUCT)
				{
					i += STRUCT.size() - 1;
				}
				else if(str.substr(i, CLASS.size()) == CLASS)
				{
					i += CLASS.size() - 1;
				}
				else
#endif
				  if(str[i] == '<')
				{
					if(buffer) buffer[total_size] = str[i];
					++total_size;
				}
				else if(str[i] == '>')
				{
					size_t to_modify = total_size;
					if(str[i - 1] != ' ')
					{
						++total_size;
					}
					else
					{
						to_modify = total_size - 1;
					}
					if(buffer) buffer[to_modify] = str[i];
				}
				else
				{
					if(buffer) buffer[total_size] = str[i];
					++total_size;
					if(str[i] == ',' && str[i + 1] == ' ')
					{
						++i;
					}
				}
			}
			return total_size;
		}

		template <typename T>
		consteval auto stringify_typename()
		{
			constexpr auto str	  = typename_signature_offset {}.transform<get_signature<T>()>();
			constexpr auto size	  = get_scope_impl(str);
			constexpr auto result = []<size_t size>(std::string_view str) constexpr {
				std::array<char, size> result {};
				get_scope_impl(str, result.data());
				return result;
			}.template operator()<size>(str);
			return fixed_string {result};
		}

		template <auto KnownValue>
		consteval auto get_known_offset() -> size_t
		{
			constexpr auto Str = get_signature<KnownValue>();
			if(Str.size() == 0) throw std::exception();
			size_t depth {0};
#if defined(STRTYPE_MSVC)
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
#elif defined(STRTYPE_GNUG)
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
#if defined(STRTYPE_MSVC)
			constexpr auto end_offset = 7;	  // sizeof(">(void)")
#elif defined(STRTYPE_GNUG)
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

	struct sequential_searcher;

	/// \brief Customization point for enum types
	/// \details Here you can add a customization for your specific enum type, set its `BEGIN` and `END`, as well as custom searcher, or search depth overrides.
	/// \tparam T A valid stringifyable enum type (a scoped enum that satisfies std::is_integral)
	template <details::IsValidStringifyableEnum T>
	struct enum_information : public details::enum_start_t<T>, details::enum_end_t<T>
	{
		using SEARCHER = sequential_searcher;
	};

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

		template <typename T, typename Y, typename... Res>
		struct get_array_pack_size<std::pair<T, Y>, Res...> : get_array_pack_size<T, Res...>
		{};
	}	 // namespace details

	/// \brief iterates over the indices, and fills in the values as needed.
	/// \returns an std::pair<std::array<std::string_view>, std::array<T>>. The arrays are 1-1 mapped (i.e. the indices point to the same enum value, just different representation)
	/// \note this function is solely provided to make implementing custom *_searcher types easier.
	template <typename T, typename sequence_type, auto... Indices>
	constexpr auto stringify(std::integer_sequence<sequence_type, Indices...>)
	{
		constexpr auto get_and_fill_valid_enum_values = [](std::string_view* str_buffer = nullptr,
														   T* value_buffer				= nullptr) constexpr {
			size_t count = 0;

			constexpr auto get_and_fill_valid_enum_value =
			  []<auto Index>(auto& count, std::string_view* str_buffer, T* value_buffer) constexpr
			{
				constexpr auto get_enum_name = []<T value>() constexpr {
					return details::stringify_value_impl<value,
														 details::get_known_offset<T {enum_information<T>::BEGIN}>()>();
				};

				constexpr auto enum_value {T {static_cast<std::underlying_type_t<T>>(Index)}};
				constexpr auto name = get_enum_name.template operator()<enum_value>();
				if(!name.empty())
				{
					if(str_buffer)
					{
						value_buffer[count] = enum_value;
						// workaround for MSVC related ICE
						// todo report the ICE, and verify for fix later
						[]<details::fixed_string Str>(size_t index, auto& str_buffer) constexpr {
							str_buffer[index] = Str;
						}.template operator()<name>(count, str_buffer);
					}
					++count;
				}
			};

			(get_and_fill_valid_enum_value.template operator()<Indices>(count, str_buffer, value_buffer), ...);
			return count;
		};
		constexpr auto size = get_and_fill_valid_enum_values();

		std::array<std::string_view, size> str_result {};
		std::array<T, size> val_result {};
		get_and_fill_valid_enum_values(str_result.data(), val_result.data());
		return std::pair {str_result, val_result};
	}

	/// \brief Sequentially searches from [Begin, End) for valid enum values
	/// \note this can be quite compile time intensive O(max_size()), and comes with an extra limitation for CLang if the fold expression exceeds 256 values. An additional indirection was added to relax this, but do keep it in mind. See the searchers for example usage.
	struct sequential_searcher
	{
		template <typename T, auto Begin, auto End>
		consteval auto max_size() -> size_t
		{
			return End - Begin;
		}

		template <typename T, auto Begin, auto End>
		consteval auto operator()() const noexcept
		{
			using underlying_t = std::underlying_type_t<T>;
			// we need this workaround for CLang which has a hard limit of 256 expansions for fold expressions
			constexpr underlying_t PACK_SIZE  = 255;
			constexpr underlying_t remainder  = (End - Begin) % PACK_SIZE;
			constexpr underlying_t iterations = (End - Begin - remainder) / PACK_SIZE;

			constexpr auto split_into_iteration_packs_and_invoke = []<std::underlying_type_t<T>... Indices>(
			  std::integer_sequence<std::underlying_type_t<T>, Indices...>) constexpr
			{
				constexpr auto merge_results = []<typename... Ts>(Ts&&... arrays) constexpr {
					constexpr auto total_size = details::get_array_pack_size<Ts...>::value;
					std::array<std::string_view, total_size> res_string {};
					std::array<T, total_size> res_values {};
					size_t offset {0};
					constexpr auto fill =
					  [](auto& dst_str, auto& dst_values, const auto& src, size_t& offset) constexpr {
						  for(size_t i = 0; i < src.first.size(); ++offset, ++i)
						  {
							  dst_str[offset]	 = src.first[i];
							  dst_values[offset] = src.second[i];
						  }
					  };
					(fill(res_string, res_values, arrays, offset), ...);
					return std::pair {res_string, res_values};
				};

				// returns all valid enum values for the given range [Offset, Offset + Count) as a
				return merge_results(stringify<T>(details::make_offset_sequence<Begin + (Indices * PACK_SIZE),
																				PACK_SIZE,
																				std::underlying_type_t<T>>())...,
									 stringify<T>(details::make_offset_sequence<Begin + (iterations * PACK_SIZE),
																				remainder,
																				std::underlying_type_t<T>>()));
			};
			return split_into_iteration_packs_and_invoke(
			  std::make_integer_sequence<std::underlying_type_t<T>, iterations>());
		}
	};

	/// \brief searcher specialized for enums that are used as bitflags.
	/// \note does not include combinatorial values (like 0x3, which would be bit flag 1 && 2). If that's needed, consider `strtype::sequential_searcher`
	struct bitflag_searcher
	{
		template <typename T, auto Begin, auto End>
		consteval auto max_size() -> size_t
		{
			return (sizeof(std::underlying_type_t<T>) * 8) + 1;
		}

		template <typename T, auto Begin, auto End>
		consteval auto operator()() const noexcept
		{
			using underlying_t	= std::underlying_type_t<T>;
			using index_t		= size_t;
			constexpr auto BITS = sizeof(underlying_t) * 8;
			static_assert(BITS <= sizeof(size_t) * 8, "No support for larger than `sizeof(size_t)` bytes");

			// prepares an std::index_sequence<> where the indices are every bit value with added 0. I.e. it's a range
			// that looks like: { 0, 1, 2, 4, 8, 16, 32, ..., 1 << (sizeof(underlying_t) * 8) }
			constexpr auto bit_shift_indices = []<size_t... Indices>(std::index_sequence<Indices...>) constexpr
			{
				return std::index_sequence<0, size_t {1} << Indices...> {};
			}
			(std::make_index_sequence<BITS>());

			return stringify<T>(bit_shift_indices);
		}
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
				return STRTYPE_MAX_SEARCH_SIZE;
			}
		}

		template <typename T, auto Begin, auto End, typename Searcher, size_t KnownOffset, bool OnlyStrings = true>
		consteval auto get_unique_entries()
		{
			constexpr auto MAXSIZE = Searcher {}.template max_size<T, Begin, End>();
			static_assert(
			  MAXSIZE <= max_search_size<T>(),
			  "Up the max search size for this enum. Either prefferably by specializing the 'enum_information<T>' and "
			  "setting MAX_SEARCH_SIZE higher, or by upping the define (which would globally increase it)");

			constexpr auto result = Searcher {}.template operator()<T, Begin, End>();
			static_assert(IsRequiredReturnType<T, std::remove_cvref_t<decltype(result)>>,
						  "the result type should be of `std::pair<std::array<std::string_view, SIZE>, std::array<T, "
						  "SIZE>>` from the searcher.");

			static_assert(result.first.size() <= MAXSIZE,
						  "Expected the Searcher{}.max_size() to be larger or equal to the resulting size");
			if constexpr(OnlyStrings)
			{
				return result.first;
			}
			else
			{
				return result;
			}
		}
	}	 // namespace details

	/// \brief Compile time stringify your enum type into an array from the range BEGIN to END
	/// \tparam Searcher functional object that can iterate, and return the values (see `strtype::sequential_searcher` for example)
	/// \tparam T enum type that satisfies the constraint
	/// \tparam Begin start of the range (inclusive) to stringify
	/// \tparam End end of the range (inclusive if decltype(End) == T, otherwise exclusive)
	/// \returns an `std::array<std::string_view>` containing the ordered (by std::underlying_type_t<T>) string based representation values of the enum.
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

	/// \brief Compile time stringify your enum into an associative container from the range BEGIN to END
	/// \tparam Searcher Searcher functional object that can iterate, and return the values (see `strtype::sequential_searcher` for example)
	/// \tparam T enum type that satisfies the constraint
	/// \tparam Begin start of the range (inclusive) to stringify
	/// \tparam End end of the range (inclusive if decltype(End) == T, otherwise exclusive)
	/// \returns an associative container where you can either iterate over the enum values as an std::pair<std::string_view, T>, or where you can use the bracker operator to efficiently convert string to T and vice-versa.
	template <details::IsValidStringifyableEnum T,
			  auto Begin		= enum_information<T>::BEGIN,
			  auto End			= enum_information<T>::END,
			  typename Searcher = typename enum_information<T>::SEARCHER>
	consteval auto stringify_map()
	{
		constexpr auto begin = details::guarantee_is_underlying_value<T, Begin>();
		constexpr auto end	 = details::guarantee_is_underlying_value<T, End, true>();
		static_assert(begin < end, "The end value should be larger than begin");
		constexpr auto values_pair =
		  details::get_unique_entries<T, begin, end, Searcher, details::get_known_offset<T {Begin}>(), false>();
		return details::ct_bst<T, values_pair.first.size()>(values_pair.first, values_pair.second);
	}

	template <typename T>
		requires(!details::IsValidStringifyableEnum<T>)
	consteval auto stringify()
	{
		return details::stringify_typename<T>();
	}

	template <typename T>
	consteval auto stringify_typename()
	{
		return details::stringify_typename<T>();
	}
}	 // namespace strtype

#undef STRTYPE_MSVC
#undef STRTYPE_GNUG
#undef STRTYPE_SIG
