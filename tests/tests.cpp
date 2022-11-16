#include "strenum/strenum.hpp"

#include <iostream>

enum class foo
{
	foo,
	bar,
	sin = bar + 5,
	cos,
	tan,
	_BEGIN = foo,
	_END   = tan,
};

enum class foo2
{
	foo,
	bar,
	sin = bar + 5,
	cos,
	tan
};

int main()
{
	constexpr auto foo_str	= strenum::stringify<foo, foo::foo>();
	constexpr auto foo2_str = strenum::stringify<foo2, foo2::foo, foo2::tan>();
	for(auto str : foo2_str) std::cout << str << std::endl;
	for(auto str : foo_str) std::cout << str << std::endl;
	return 1;
}