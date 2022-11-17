#include "strenum/strenum.hpp"

#include <iostream>

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

enum class foo_with_hole
{
	foo,
	bar,
	sin = bar + 5,
	cos,
	tan
};

enum class bit_ops : unsigned char
{
	NONE	   = 0,
	BIT		   = 1 << 0,
	SHIFT	   = 1 << 1,
	ARITHMETIC = 1 << 2,
	LOGICAL	   = 1 << 3
};

enum unscoped_foo
{
	unscoped_bar,
	unscoped_sin = unscoped_bar + 5,
	unscoped_cos,
	unscoped_tan
};

int main()
{
	/*constexpr auto foo_str		= strenum::stringify<foo>();
	constexpr auto foo2_str		= strenum::stringify<foo2, foo2::foo, foo2::tan>();
	constexpr auto enum_ops_str = strenum::stringify<enum_ops_t, enum_ops_t::NONE, enum_ops_t::LOGICAL>();
	for(auto str : foo2_str) std::cout << str << std::endl;
	for(auto str : foo_str) std::cout << str << std::endl;*/

	constexpr auto v = strenum::stringify<bit_ops, bit_ops::NONE, bit_ops::LOGICAL>();
	constexpr auto c = strenum::stringify<unscoped_foo, unscoped_bar, unscoped_tan>();
	return 1;
}
