#include "strtype/strtype.hpp"

#include <catch2/catch_test_macros.hpp>

enum class foo_known_size
{
	foo,
	bar,
	sin = bar + 5,
	cos,
	tan,
	_BEGIN = foo,
	_END   = tan,
};

TEST_CASE("basic enum without specialization")
{
	constexpr auto values				= strtype::stringify<foo_known_size>();
	constexpr std::array correct_values = {"foo", "bar", "sin", "cos", "tan"};
	STATIC_REQUIRE(values.size() == 5);
	STATIC_REQUIRE(values.size() == correct_values.size());
	STATIC_REQUIRE(correct_values[0] == values[0]);
	STATIC_REQUIRE(correct_values[1] == values[1]);
	STATIC_REQUIRE(correct_values[2] == values[2]);
	STATIC_REQUIRE(correct_values[3] == values[3]);
	STATIC_REQUIRE(correct_values[4] == values[4]);
}

enum class foo_with_hole
{
	foo,
	bar,
	sin = bar + 5,
	cos,
	tan
};

namespace strtype
{
	template <>
	struct enum_information<foo_with_hole>
	{
		using SEARCHER				= strtype::sequential_searcher;
		static constexpr auto BEGIN = foo_with_hole::foo;
		static constexpr auto END	= foo_with_hole::tan;
	};
}	 // namespace strtype

TEST_CASE("basic enum with specialization and hole")
{
	constexpr auto values				= strtype::stringify<foo_with_hole>();
	constexpr std::array correct_values = {"foo", "bar", "sin", "cos", "tan"};
	STATIC_REQUIRE(values.size() == 5);
	STATIC_REQUIRE(values.size() == correct_values.size());
	STATIC_REQUIRE(correct_values[0] == values[0]);
	STATIC_REQUIRE(correct_values[1] == values[1]);
	STATIC_REQUIRE(correct_values[2] == values[2]);
	STATIC_REQUIRE(correct_values[3] == values[3]);
	STATIC_REQUIRE(correct_values[4] == values[4]);
}

enum class unreasonably_large : std::int64_t
{
	first	   = -2000,
	some_other = -51,
	then_more  = 20,
	and_more   = 2000,
};

namespace strtype
{
	template <>
	struct enum_information<unreasonably_large>
	{
		using SEARCHER							= strtype::sequential_searcher;
		static constexpr auto BEGIN				= unreasonably_large::first;
		static constexpr auto END				= unreasonably_large::and_more;
		static constexpr size_t MAX_SEARCH_SIZE = 8000;
	};
}	 // namespace strtype

TEST_CASE("large enum with extended search size")
{
	constexpr auto values				= strtype::stringify<unreasonably_large>();
	constexpr std::array correct_values = {"first", "some_other", "then_more", "and_more"};
	STATIC_REQUIRE(values.size() == 4);
	STATIC_REQUIRE(values.size() == correct_values.size());
	STATIC_REQUIRE(correct_values[0] == values[0]);
	STATIC_REQUIRE(correct_values[1] == values[1]);
	STATIC_REQUIRE(correct_values[2] == values[2]);
	STATIC_REQUIRE(correct_values[3] == values[3]);
}

enum class bit_ops : std::uint64_t
{
	NONE	   = 0,
	BIT		   = 1 << 0,
	SHIFT	   = 1 << 1,
	ARITHMETIC = 1 << 2,
	LOGICAL	   = std::uint64_t {1} << 63,
};

namespace strtype
{
	template <>
	struct enum_information<bit_ops>
	{
		using SEARCHER				= strtype::bitflag_searcher;
		static constexpr auto BEGIN = bit_ops::NONE;
		static constexpr auto END	= bit_ops::LOGICAL;
	};
}	 // namespace strtype

TEST_CASE("bitflag enum")
{
	constexpr auto values				= strtype::stringify<bit_ops>();
	constexpr std::array correct_values = {"NONE", "BIT", "SHIFT", "ARITHMETIC", "LOGICAL"};
	STATIC_REQUIRE(values.size() == 5);
	STATIC_REQUIRE(values.size() == correct_values.size());
	STATIC_REQUIRE(correct_values[0] == values[0]);
	STATIC_REQUIRE(correct_values[1] == values[1]);
	STATIC_REQUIRE(correct_values[2] == values[2]);
	STATIC_REQUIRE(correct_values[3] == values[3]);
	STATIC_REQUIRE(correct_values[4] == values[4]);
}

enum unscoped_foo
{
	unscoped_bar,
	unscoped_sin = unscoped_bar + 5,
	unscoped_cos,
	unscoped_tan
};

TEST_CASE("unscoped enum should be rejected") { STATIC_REQUIRE(!strtype::details::is_scoped_enum_v<unscoped_foo>); }


TEST_CASE("compile_time lookup")
{
	constexpr auto map = strtype::stringify_map<bit_ops>();
	STATIC_REQUIRE(map[bit_ops::LOGICAL] == "LOGICAL");
	STATIC_REQUIRE(map["LOGICAL"] == bit_ops::LOGICAL);
}

TEST_CASE("runtime lookup")
{
	constexpr auto map = strtype::stringify_map<bit_ops>();
	REQUIRE(map[bit_ops::LOGICAL] == "LOGICAL");
	REQUIRE(map["LOGICAL"] == bit_ops::LOGICAL);
}


namespace foos::dor::ri
{
	template <typename T>
	struct foobari
	{};
}	 // namespace foos::dor::ri


TEST_CASE("stringify typename")
{
	using namespace foos::dor::ri;
	constexpr auto v = strtype::stringify<foobari<int>>();
	STATIC_REQUIRE(v == std::string_view {"foos::dor::ri::foobari<int>"});
	STATIC_REQUIRE(strtype::stringify<foobari<foobari<int>>>() ==
				   std::string_view {"foos::dor::ri::foobari<foos::dor::ri::foobari<int>>"});
	STATIC_REQUIRE(strtype::is_templated_type<foobari<int>>() == true);
	STATIC_REQUIRE(strtype::is_templated_type<int>() == false);
}

TEST_CASE("stringify namespace")
{
	using namespace foos::dor::ri;
	constexpr auto v = strtype::stringify<foobari<int>>();
	STATIC_REQUIRE(v == std::string_view {"foos::dor::ri"});
	STATIC_REQUIRE(strtype::stringify<foobari<foobari<int>>>() == std::string_view {"foos::dor::ri"});
	STATIC_REQUIRE(strtype::stringify_namespace<int>() == std::string_view {""});
}
