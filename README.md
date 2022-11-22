# strtype
C++20 support for stringifying enums and typenames at compile time (with constraints) for *The Big Three* compilers (GCC, CLang, and MSVC). 

## How it works
(Ab)using the compiler injected macro/extension `__FUNCSIG__` or `__PRETTY_FUNCTION__`, at compile time parse the resulting string to extract the enum value or typename. In the case of enum values we will be rejecting entries that do not point to named values.

I don't take responsibity for when this breaks, compiler provided macros and extensions can always be changed, but as of writing (and looking at historical behaviour of these 2 values) they appear to be pretty stable. As of now none of the code goes explicitly against the standard, nor relies on UB (aside from the compiler specific macro/extension usage) and so I foresee this to keep working for some time.

## Limitations
These limitations really only apply to enums, typenames do not have any limitations aside from recovering alias names. Due to how the compilers interact with alias typenames they are substituted before we can recover that information.

The enum types are currently restricted to anything that satisfies `std::is_integral`, and `std::is_scoped_enum`. The integral limitation isn't really needed, it just lowers the potential oversight of unforeseen issues. Feel free to implement the `std::is_arithmetic` constraint, add tests, and make a PR if you're up for it!

Search depth: As we need to iterate over all potential values that are present in the enum, and as iterating over the entire 2^n bits of the underlying type is too heavy; we limit the search depth by default to `1024`, and offer 2 different iteration techniques (`strtype::sequential_searcher`, and `strtype::bitflag_searcher`). Both of these are tweakable, see the following section for info on how to do so.

CLang has a further limitation of how many times a fold expression can be expanded (256). We have a small workaround for this implemented in the `strtype::sequential_searcher` (one layer of indirection), but it means when the range exceeds 255 * 255 on CLang, the compilation will fail unless this limit is increased.

Duplicate named values (i.e. multiple enum names on the same `value`) will only fetch the first name it sees. So when the following enum is defined:
```cpp
enum class foo {
    bar,
    tan = bar,
    cos,
    sin,
};
```
The output will be an array of `{ "bar", "cos", "sin" }`, notice the missing `"tan"` (see this [on godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjE/gAcpAAOqAqETgwe3r7%2ByanpAiFhkSwxcWaJdpgOGUIETMQEWT5%2BAVU1AnUNBEUR0bEJtvWNzTltwz2hfaUDFQCUtqhexMjsHOYAzKHI3lgA1OZmCAQESQogAPQXxEwA7gB0wIQIXlFeSiuyjAT3aCwXAClMAoFABPAAiQQuCgIjVBSUwFywADcLttdoiYXCEdDYQR4Zh7ggkklDiYNABBclUsxbBg7Lz7EwbNzIGH4VDM7DU6mMHx7HZMEF7fioA4AdislL2Mr2UQapGpsr29QYBw24LlCqVsrQCkV0tlaQYBqp4vBzKlVMpzDYCiSTFWeyx%2BIREqtMoImBYSQMXuZbi5OqxXgcez5LAA%2BqF%2BMQWExHAIA6Kue6dTL3qFgHshNgKQAlNwACWw%2BfVmpdBJAICUAEcvN88GJI0oGsgELFLennfVHMgBQIYZhVEliHsmF4iHsLNgAOIASXCyplJklItQqGr8rHq4tGw9Rt7eH7aAYQ5HY4nU%2Bw4U1y5Xa9F1eNEr3B5lVx7CePA7PXovzp4AAXpgkYEHsACyFIABqRrmBbFnB84AFrYO6exmBokjxK%2Blp7J%2BqBJImzC0LQoJ7KgyKxMQeD7AQHbOpgbYIIBIGkCqCB4AoexcQwYAcOBYSYFg6B7F4DD0MKoJLHsnHgfRmB7LQeAsIQ9w6ruXaUpp2mUqE4HxqEEBzGmhqnueo7jpOYrImIDbccyFZ4lWNawlmeBUKCyYblyxlaRSMqxhA5ngVenJmAAbD2Y4gHstneMCJmru%2Bh7oNWo76VQEBkmYACsCgmLlbgMIc7FYvc6AJkwxlzP5D4WoaxCYAQyxqho/m7hwCy0JwuW8H4HBaKQm4cIGljWM6SwrIpmw8KQBCaF1CwANb%2BOK9zxGYACc8SSGYXAbLlW1bRFXBcPonCSP1i3DZwvDnBo82LQscCwDAiAoKgPp0LE5CUH8SQ/XEwAKMwZwIKgBCkCix6YAAangmC3AA8giA1zTQtBesQ5wQFEN1RKEDSgpwc1/GwgjIxJJODbwWDxkY4i09DeBNTUVHnMzw7VJOaxzfpmA9czylRDcxCgh4WCk7wbksNLCxUAYIMI0jqOMNLMiCCIYjsFwAT8IIigqOozO6OdBhGCg1jWPoeBROckALIRxGcwAtOyjmmONliYXsrvIxs92C9UxEuAw7ieC0ejBFMJRlHoKRpMRox%2BOdicFAwvRxwM53tMRXQjJHOS58HHQMAXkzFP0cS5xMKd6DC3RZ9XEgLAoU2rK3F0cH1pADUNI17Ko8QRa7EWSHswDIP2ECwuJy0mRAuCECQBy0lwcwy89CwdkwWBxMZpCrVwuUbadZgX7lkgaBf4qSCd3dXaQcsRWY9xcBoZ0ReKGwn1w4q5XOv3XgI17ogEegtWmL13oQCQNzZA1kyAUAgA0EGyhDCCyEBDW46NeAAzoN%2BAQ6Cwi0CwagHBN18H0GBqDJg4NIakCobEZGk4yEUO4LweBFJiAgzuoEVQ1Q6j4AGrwA2whRDiCkJreQSg1A3V0BsfQhhjDW0sLbe28AnZEQyG7D2GovZWB9kNduyxO612EcQzB2DcHzSanzXgtwbhJHlt3XuwDbocGwAIhBRAxzD1HuPSe089iz2IPPReY1DE2D2MvXxa9f6byelAneTF96UG6pdXgcsNjrXiDteIGxIq7U2vfPuN1QG2HAUkrQcwVr%2BEkPcLaFRJAbA2BoV%2BZgWmFO7oHMpzMKmQJqaQV6UAYFICYUg/6X1AbUJQBbYAYEwkMGWtDTAyJYYqxRmjDWmNsa43xszQmzBxYa3Jt8KmZEbr02UUzIa%2BA2aOA5jdeBvMNYCyFkNEWYsJYYHsbYlSLjFZMGVojLZ6sOHSO1pI/WsgjZyNNiARR8yrbexsCLB2h9nY6M4O7Ag6BPaqIsL7f2vS84ZDDhHbIqdAjh2bjMGueQk4ZHrmnfIxE6XxxLvYfOdci7UrJZ0CYHKc5DG6Cy0VjRhU1zbh3XWGSe7XX6ZwIeI8x4T0FEYEJc9lmL1iavWaiTBlLVILvNJh9VotPuFfPWuUr5bVyu07a4pH5ZJAK/d%2Bn8uDf1/oAgBQDyl8IetUrqwyxkgBeb4v6KCeEKEsaQ6xpzpkEOInGth7iJlzOUYs%2BejCk3UJYcIBNEKuExr4fAoRoQ%2BFiKhbraQYi4UmyGmbJRlsDE23RZo4a2jBw4r0eCNtRj7qyobni0Iqai1zVhJgP5ji6EuKFm4gNnjvGIJVQE9V8ytVLIXiEyJNsYn4DiQareySTWpIGIfIWT9sm5PyYUiKxTOlbT6QPQNVSjW1KPv4N%2BDqr55RyeKMw4p4i5Q2IkIWvT3EDOeqG%2BAIzYGfW%2BtQqNGbkTIBJJGZEXAtqRj9MCAgkZVDj1Wes1Ymy1Y2N2bEfZBMiYnIhWcym1MrnehuWsO5rMQ54CeVzFdXo3mCEFjdL5xNJbsZljROWHCFZKwUOR7ZELq0SNrdIht8iQDSGRQOtFdsMVaJdpwXEaVtO%2BwuMjMw%2BFwS3mwBYOQs58LITwPhWc5ELizgAGL4WRsoAAKiAecbhPNmd8yAfM2AgtY2QMAIO3LyUQFcOKmOVd6UJzZcyvlqWmWFFji3LlXHBVioy3lsuFcpUjsK1S8rkqcspY3osUxcrXGKtfRwIegSWAKDQ3FbD9w8MwhCXqnc69DXby/a0%2B4rTJtTemy65%2Bbq34fy/j/P%2BfqX0gLfRAmD8G4F8ZIFG1BsaMHxvITYqhhCGDjpO5QvNAw0MYawzhvrBGiPSAmQWtNGsS28I4Jw7xFaRGQuUxIOtsLZGNp0LkLThL1F6a7QZjgRmUVRMwkOhrI6LFHY%2BxCqdM6nHzt6s19by6eZxJe3sDrXWHu9YTPh3d0OD0ryGwkk9NT5XXsRbe%2BIBSinYSfWtjxYDNuntWp0%2B4P9X7HT2pIXnWFco9MJwL4Nn6hZmAV9B09VEcbkskEAA%3D%3D%3D)). This is because during compilation `foo::tan` is considered an alias of `foo::bar`, and will be substituted. There is no way of recovering this information.

## Usage & customization points
The two entry functions into stringifying your enums/types are `strtype::stringify<TYPE/VALUE>()` and `strtype::stringify_map<YOUR_ENUM_TYPE>()`.
The first function will return you an `std::array<std::string_view>` if given an enum type, otherwise when given an enum value it will return you a `std::string_view` representation of the enum value. In the case of the array return the values are sorted based on the underlying enum values.
The `stringify_map` function will return you a compile and runtime searchable associative container where you can search for the enum value based on its string representation and vice-versa.

Your enums should either come with a `_BEGIN`/`_END` sentinel values in the enum declaration, or you should specialize the `strtype::enum_information` customization point (see example section). Note that both the specialized `END` and the embedded `_END` act as **inclusive limits to the range**. This means unlike normal ranges, which are exclusive ranges, the endpoint is used as the last value. This is the mathematical difference of `[0,10]` (range of 0 to 10, inclusive) and `[0,10)` (a range of 0 to 9, excluding 10). This was done for convenience so that users don't need to define `END` as `END = some_value + 1`. This is *only* the case when within the enum declaration scope, or when `END` is set as an instance of the enum type object; if it's set as its underlying type then it behaves like an exclusive range limitter again.

By default the search iterations is limited to `1024`, this means if the difference between the first and last enum value is larger than that, you'll either have to specialize `strtype::enum_information` for your type, or globally override the default value by defining `strtype_MAX_SEARCH_SIZE` with a higher value.

Lastly the search pattern. There are 2 provided search patterns `strtype::sequential_searcher` and `strtype::bitflag_searcher`. Both will search from `_BEGIN` to `_END`, but have a different approach.
- `sequential_searcher`: iterates over the range by adding the lowest integral increment for the underlying type from `BEGIN` to `END`.
- `bitflag_searcher`: iterates over the range by jumping per bit value instead (so an 8bit type will have 8 iterations, one for every bit + the 0 value). Combinatorial values are not searched for. For example if there is a value at 0x3, which would be both first and second bit set, it would be skipped.

You can provide your own searcher, as long as it satisfies the following API:
```cpp
struct custom_searcher {
  template<typename T, auto Begin, auto End>
  consteval auto max_size() const noexcept -> size_t { /* return the theorethical max value for your enum type */ }

  template<typename T, auto Begin, auto End>
  consteval auto operator() const noexcept -> std::array<std::string_view, /* size must be calculated internally */>
};
```
See `strtype::sequential_searcher`, or `strtype::bitflag_searcher` for example implementations. Note that at least the `sequential_searcher` has some compiler specific performance optimizations and workarounds which do complicate the code a bit.

## Examples

### compile time stringify an enum ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjE/lykAA6oCoRODB7evv5JKWkCIWGRLDFxZgl2mA7pQgRMxASZPn4BldUCtfUEhRHRsfG2dQ1N2a1D3aG9Jf3lAJS2qF7EyOwc5gDMocjeWADU5mYIBASJCiAA9OfETADuAHTAhAheUV5Ky7KMBHdoLOcAUpgFAoAJ4AESC5wUBAaIMSmHOWAAbuctjsEdDYfCoTCCHDMHcEIlEgcTBoAIJk8mMHy7bZMYG7fiofYAdisFN2uyi9VIuzqDD5aAUfNSgqpJlZYJM6w5lIpzDYCkSTBWu0xePhbLlXIImBYiQMeplbhl2CpXMxXgcuxpLAA%2BqF%2BMQWExHAITcyzdqLVzdm9QsBdkJsOSAEpuAAS2DD%2B3WYPVuPxIBASgAjl4vngxPalPVkAhYjKdX7oW68Mg6QJoZhVIliLsmF4iLsLNgAOIASXCfr9kosTNQqBTPIbkulst9lrqjkraAYNbrDabLew4QTvb77MHw9ToTZE5LXMuifLc%2BreqX6rwAC9MPaCLsALLkgAa9pD4ajH87AC1sNquxmBokgABwHsWuwnqgiTuswtC0CCuyoEisTEHgewEIW6qYPmCDXnefJYXgCi7CRDBgBwj5hJgWDoP6DD0IyIKLLsCCEPy2G0HgLCEHcvrjsWEpShKFKhI%2BrqhBAsw%2BpyVYLpe9aNs2LJImImakTKCYasmqYwoGeBUCCnpDma0lCXJZazvaDLvAQEBqd4QImAArBYGiuQmWlafsZhmKOByzBZ5JcsQmAEEsDC7B5k4UuOHDzLQnAubwfgcFopDDhwpqWNY6qLMsmC%2BesPCkAQmgJfMADW/isncoFmAAnKBkjlOsLmNY1ABsXAJElHCSKlFWZZwvBnBoZUVfMcCwDAiAoKgBp0LE5CUL8iTLXEwAKMwpwIKgBCkMiFaYAAangmA3AA8vCaWlTQtB6sQZwQFEw1RKE9QgpwpW/GwghXYx33pbwWCukY4gg0deBhdUqFnFDtZVM2qyleJmD9Rl3FRNcxAgh4WA/bw%2BksET8xUAY23nZdN2METMiCCIYjsFwAT8IIigqOoUO6H1hjGNY1j6HgURnJA8wwXBCMALTQugWmmLlljAbs0tXesY0Y1UcEuAw7ieM0ejBJMxSlHoySpHBIx%2BAkFv5AwPSm/0FRa%2B0DCdMMBvZC79hwR7ExFH0cQVOM1t6GWDSO0HEjzAoBUrDH%2BjJUNUNZbsqigV10tdZIuzAMglYQDCXgMFVMkQLghAkMVXCzMTU3zIWTBYHE0mkDVXAufVPV%2BWYLmSBofmspI3VJwNvCk11Zh3FwGi9V1rIlS5XCssvpBpRlWVjSAE3lSD01zRASBI8gKlkBQED1NtyiGBjQj7Tcd28OtdDlgIN9hLQ9%2BoI/w0v/QW0dpMD2gdUg/9YhXWbN/X%2BiNVBVHJMQbao1AhwOQLUfAaVeDs2EKIcQUgGbyCUGoYauh1j6H5igQWlhhai3gBLWC6QZZywVlQiwwExrxxZoMDBH874PyfmVMKqNeA3GuIkMmY8Urr2GllbAqCz7p0ztnXO%2BdC7F1LuXHKVhqG7ErkQMcZgSp10mvvRuuEW6UESpwQapBSbrDqqBZqoF1hmC6i1BqI9pGp2QeNExWhZjVX8JIO4jUzAtXWOsDQU8zCSAiQEfqGsvGb2QXvfxpAZpQEPkgcB581qLQ2gAlABgjAPmICXKqR1MBIhOtTa6t16YPSei9N6UMPrMDxvTP6XxAaIWGmDfmkMMr4Fho4eGw0T4o3pujTGvBsa43xhgYRgieISIpkwKmF06l024Fg2QTM8Fs1kJzYhPMQBkOKQLJWNhsZizbpLRhnBZYEHlvGRW2i2EaFVurTWvt0i631lkG2gQ9ZR2mMHXIlt0hh1tnkOCoKzY%2B21jUUOXsgVtD9uMeFztBhdGhTiyOJto61wWEsBOxL%2BpSI3rwNOGcs45zpBc3YRcykaKZXo6uGxa711MaQJuFi241ViXcfurMXL90ai5KJTVWRjxsZPaes956L07ivNeVKRocG3rvKa6SskgAmfo1al9EEKF4V/fhnT8mvzgma6B6qclFP5qU8pYCrUAMgcIC1OyUHwJNcgk%2B6DQjIOwfslm0hsHHO5hlXm5CjCUKuTQ259CpaPOYa81h7CSWFXDs80ItqvWlRhJgJZojgESIpSnZJHA5HI30Youlud6RGCZeosuTKtFC10fgOtnLjGpMqry8x/Q279TlWchxTiXFuLAjExqSTqU%2BJ3n4gdNU/LCo0P3Pu9jWRmFZKBFy6xQJj0Seqrey6D7wAyUfBaS0AFGodUiZAxJ7RIi4I1e0RogQEHtKoHOlTqkrFqbTARjTYjNPep9Dp3qukAyBn0/UAzVhDJhkisZsDa1LOmcNOZX0CZIeJuhUmOzyaUwUEB%2Bp3qQ24LDQQyNJCQDSAufG95ia6GZQYdWTgOJ0DMesCrc4V0zBQTBOubAFg5Dtigr%2BPAUF2xIXOO2AAYlBK6ygAAqIBOxuGUwJ9TIAwzYB049ZAwAflIucBAVweLjaBzBebWFULUX2chQUQldnEVu39ni9FyKuhYvBRHRoTmQ5%2Bbcwi2OnDE4VvnRq9O9KWAKEfbsV9jU7ifuhGy7tHLDFcvPYEiJdwIlFeKyV2VE8QBTxnnPLgC8l6qoSKexd2r966svcfeRhqL5X1Nbfc1P8BH/zfgwfN/W/5uv6I%2B59KWP1ui/T%2Bv9OSPV2vpifBBSDNU%2BrQbmzBBDQ0SHDUcohUadA5CY28oWNy2P3M4xwbjvHlYZTjqSrhcs829eW96otJaxHluTjF2RHXq6/tzglpLKW0uzYyxADtOj2UGKMdy/xVjx62PHfVSdrj3Gzv%2B01vL7cgl3EXlPTqrVJAzpAi5Y9laF0bf7QEseZhqcasRwO1Cz0/mSCAA%3D%3D%3D))
Following example showcases an enum being stringified, and accessible as a contiguous array of `std::string_view`'s.
```cpp
enum class foo {
  bar, tan, cos, sin,
};

namespace strtype {
  template<>
  struct enum_information<foo> {
    using SEARCHER = strtype::sequential_searcher;
    static constexpr auto BEGIN      { foo::bar };
    static constexpr auto END        { foo::sin };
    // static constexpr size_t MAX_SEARCH_SIZE { 2048 }; // optionally override the search size, this isn't needed unless you hit the limit.
  };
}

int main() {
  constexpr auto values = strtype::stringify<foo>();
  static_assert(values[0] == "bar");
  return 0;
}
```

### compile time stringify a single enum value ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjEEgEADqgKhE4MHt6%2BekkpjgIhYZEsMXEAzLaY9nkMQgRMxAQZPn4BdpgOabX1BAUR0bHxtnUNTVlcQ929RSUgpQCUtqhexMjsHOalocjeWADU5mYIBAQJCiAA9OfETADuAHTAhAheUV5KK7KMBHdoLOcAUpgFAoAJ4AESC5wUBAaIISmHOWAAbuctjsEdDYfCoTCCHDMHcEAkEgcTBoAIJk8mMHy7bZMYG7fiofYAdisFN2uyi9VIuzqDD5aAUfJSgqpJlZYJMpQ5lIpoQIuxYTFCEDmbLlXMuuwYqAI7H5CEwu2ImAIywY/Pxu1QVCZqFQPOIAH1Mbs8ApdkxrfC6bJVQxQsAjSbMcHvV7wgAVaPKb3EYA%2BL53KlctAMaGYVQJYjerxEB1O%2BpumH7Upg3aYvHwkAgcNGPBUEEytzMuvOmXYdUyrVVuqOZAuhnvAjUR3O0t5mXSiv99B1hvAF1IvCYG6a/ZmMyd7dssFzXtp03my27DRHimSsEcBa0TgAVl4fg4WlIqE4bms1irSxWJo2HhSAITRbwWABrfxWTuAAOMwAE4YMkMwuFKB94PggA2LhxnvDhJGfUD304Xgzg0YDQIWOBYBgRAUFQFgEjoWJyEoX4mPoOJgAUZhTgQfVSGRPBVgANTXG4AHl4RfICaFoA1iDOCAoiIqJQnqEFOCA342EECSGFoTTX14LAVSMcRjMEvAzQ6JEgSI7N2gLNYgMVSoiNoPAomuYgQQ8LAtN4GE8BYQKFioAxuLE9cpMYQKZEEEQxHYLgAn4QRFBUdRLN0XDDGMb9LH0LyzkgBZUASaozg4ABaaF0BnUxLGsMwNF2GqJNKUjKnaaoXAYdxPGaPRglCPpigGcYclSARRj8Kbkhmhgpn6OJxjaDoBC6EYhrGCoqk6YYejG6ZJomHbMnm87jsKVaJAWBQ/1We79EfQjLI/DhdlUGDMJqzDJF2YBkGQXYIBhLwGHAjUIFwQgSC3UouDmILKIWY0mCwOJ1VISCuAfWDsO3MwH0kDRt1ZSQsNe/DeFCzCzDuLgNBwzDWSRh8uFZTnSBfN9PtIkByJA4yqNoiAkEc5ACxIViIHqbjlEMSohH4m4ZN4di6CYaolbCWhVdQdWiK1ziQG43iFH4ghSFN2IJILQ3jcsqXyUTareCl2p8BfXh0uEURxCkBL5CUNQiN0coDCMFBCpsTyolKnGKqqzg6oIBqKyaqxLFa0inpSoYfb1lW1Y14CzRc3gbmuBIwppp9eaIz7sFUJyiDzH6/oBoGQbBiGoZhr9mqK3Y4Y7xHkdR0X0cwTGBhxvCCNIULSmgmDEJg0ozEwpC4KppuPpI2whYomfcf8SQ7ngswkNKUoNAZsxJHvgI8K6w/%2BePkWtDF%2BBqLonbMgFAIBAJQNHZcA9wKCUwKuUS4lYrlzkgpJSKlLJqWYL5eKOkvj6UMkRUy%2BULJvnwDZRwdlqpvils5eKbk8JvgTj5PyGAq4VxCvXCKTAooIOkvFf2SUg5pVkJlcOOVZj6HyrHEe8cSrwHKpVNI1V06ZzBNnFqbUOofw2n1CArg5ojQGitCaa1SDTWqPohauQ0hGJmOtHqm0ahHQsftXqh1JgnTuutJxu0rrQncbdYxL1HrLGesjBu70v5fS7v9QG9IjD92IJDaGYNx4I0AijM%2Bv9Z7z2xhBEAL87ik1Sg%2BUm8EHyPwQqyGmy96aM2Zqzdm%2BMuY8z5rwAWJ9haUVIAAiWIBqEdzlgrBQJcDZl2wQxDiOs0gjKdq022EzmJxDiZAxJUN5mMUWQ7YQYzuCezbsgN23Fj5ewzqEY%2B/DA4pWkP7ER2U3y5QkTHNRRUE5J3kanWq9VGpxzzosEJhd6qhBmTsoCMJMCsJrkwOuuy7xvU/m0zgrd24I2iT3ZZCSklDzjnyVJ04zBIwyT/MCpAMZY0oLC2mK9Zjr03tvXekh97wXhcRDggtOnn0gtuQpGhSYkzXqyMwrIYIPlKDBGmH85ntKJX/Gi/8kBALlmApEyBiQri4PBF0BgDTQhdKoAGMC4GYGipJXhuy%2BB0BQZQNBb4MEaXGSwXSBA8FGRIZgMywBiEmWsq4ihDl9k0LNXQjyXkmH%2BTWG%2BYKoUYV8EigoY1iC%2BGyAEVckOtyI75MeQVaRxVE5yPfAogQ1UcQLmeRYVquxzgSTMBWsE4QwTYAsHIAA4hWgAWngCtTaQSdoAGIVoksoaMIAACSbg%2B2VsHSAAAStgcd8lkDAG6gdZwuiBrONGgE2xpjFrmJ8dkHd1iPGBLscuxx3RnHaLcQ0GxZ0/EXWGl4/x41bEPQLi9PCjdJWcG%2Bj3FgChlW7CROqu4WqgRKlhvgCe6Tp5ZIvvfO499ENIeQ9UumIAGZMxZlwNmHNmnjC/ayjpmTbzdPFpLf1AyQFDOBUbcupspkCBo87N8SqVUJDVRq0DOq9XSCAVs2Z8VXbu2Ofs72ZzCMXOShIa5wiw53J0P4TNUic4yNzWVfNHzi3KfUfnf5ehAUMCY%2BXMFELa71w/REhFHAkXSwnjx5U/7QZAfgiBnWYGwbDxUziyDaT8VT2I3MClNTqWwVpTvPez8mUEbZQFvJz87jswZhhZCkgGWtUkA%2BcVlmWUweJXhMw2WpVo1IHZRSaR8lAA%3D))
Here we stringify only a single value of the enum, note the abscence of the customization point as it's unneeded.
The return type of this function is a unique type that holds the string as a NTTP.
```cpp
enum class foo {
  bar, tan, cos, sin,
};

int main() {
  // note: the return type of foobar_str is a type containing the string as NTTP argument.
  constexpr auto foobar_str = strtype::stringify<foo::bar>();
  static_assert(foobar_str == std::string_view { "bar" });
  return 0;
}
```

### runtime stringify enum values ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjE/lykAA6oCoRODB7evnrJqY4CIWGRLDFxAMy2mPb5DEIETMQEmT5%2BAXaYDul1DQSFEdGx8bb1jc3ZCQojvaH9JYNlAJS2qF7EyOwc5mWhyN5YANTmZggEBIkKIAD0l8RMAO4AdMCECF5RXkprsowED2gslwAUpgFAoAJ4AESCl0mjTBiUwlywADdLjs9ojYQR4ZiCHCEQ8EIlEkcTBoAIJkylmbYMXZeA4mMpuW4MdCoFhM7BUrbohmYQ7M5CTfCoLlUxg%2Bfa7Jig/b8VCHADsVgp%2B32UQapH29QY2rQCm1qT1%2BwA%2BhZsABxACS4UFEI1WrN2HCDqZDuNpB5SohTNVlIpaAYk0wqkSxH2TC8RHlqFQppYTES9v2WJxIBAsNCzyoYITSaZbgVXIgCz9PIpoQI%2B0ToVLyv96pFGdZ7JYppReHW%2B2I6HLatTBHQGZYBC4AE5x2UlTWCBBe6Wy2VG4PhyAGrcwYWFdrJFzY/HkWIvCCTCqFRnNWQDxnddqLygUve4xnjcrfcuqU2hxmvAw8PwxDtlWHZ4Fm7w1IWqQAF6YKaBD7sQbIQBoz6HseIIPDBmD1gAtPsXBLiugEQFGMZ4CmGh%2BvsFGFvsZRmNR1jWHgZYDmeK7fmu4ZVlQECkmYACsCgmIJbgMEcaH5iSgkWAqppHt4p6yUh6AQKOZaCb6WkPOgTD1Iu/bkuqZ6%2BgOxCYAQqwMPsVGfhSpkcEstCcIJvB%2BBwWikKgnBuMxlipisawClsPCkAQmhOUsADW/hKg8AAcZjjglkhmFwZSCZOABsXAJC5HCSO5kXeZwvAXKhEWeU5pBwLAMCII%2BLCJHQsTkJQ/wtfQcTAAozDnAgqAEKQnbrAAangmB3AA8giHlhTQtAELEFwQFEJVRKEDRgpwYX/GwgjTQwtA7dVI2YImRjiGd%2BAWZ0yIgiVoYdNGGxhVWVQlbQeBRLcxBgh4WC7bweJ4CwwNLFQBi9RNU2zYwwMyIIIhiOwXABPwgiKCo6hnbo%2BWGMY/k2N9UQXJASyoIkNQXBwuEiu6piWNYZgaPsuHTWU5VVB0NQuGyYx%2BAkwQzMUpQ5CkaQCILEt5OkfRi4MEw850AjdKMngtHo7Sq7UUwKwMcQTFMMvGz0BtzEbSwKEF6wSM5rnFWdPkcPsqgJdluHZZI%2BzAMgyD7BAeJ/tFCyB7ghAkIcNKESDkULEsCCYEwWBxKWpCxVwgmJblZh54JkgaHnSqSOO2X6JwRWkOD2VmA8XAaHl2VKmUWdcEqgkJB5Xku%2BVICVfHtUNRASDPcg0YkO1pHEL1yiGFUQiDXc828J1dD6ekc9hLQi%2BoMvJVr91IC9f1CiDcNh%2BxNN0a7/vZ1j%2BSM%2B07wY91PgHm8JjwiiOIUhI/ISg1AlV0BUAwRgUDE30D9cm6cqY004PTIcjNias3KrbNGwx35bwXkvFe4ULJvV4HcW4iQIYVw4G5Ug3deAu2wKoF6RAIzu09t7X2/tA7BwYKHQOflmaWG1BHRh0dW4LDjtVBOpAk4p0GOnAqVdwbTkSilBKDFsqpSSqXKhJVe62H7uFeOMV/CSAeOOMwqUyhlA0LXMwkgLEBAKlzLRzsyr6PEUPeAdVGqXzIBQCA3iUBgOAPBYgIdzrIi7JgWGM05qI0WstYgq11pnU2swf6iN9o/COidEqWBLrAGul5W6vM8APVpl5Mer1EYfQKl5Umf0AYYEIfgsGZCoZMBhpNaJCNuCf1kCjX%2BGNZDYyAXjEAoDCYQL4STaB8BKbU3SLTRBfYygQiZlYSwrN2ac25tUdI/N3Ca3GIENkFtxYJFyFLDIhyhZJEljUU5StKi7LViba52sVY1HVtMIohttavKyDcyY5tRa/MIssVYdswUFUodQ0qrtmFex9jKIwHCQlcLDhAQRUdQqiNcVoCRUjU6UEMbYh4Bd0aCQLuOQSVjkpKnIfIkAtd66Ny4M3VuncO5d20S4iqeKaqeJHiACpjCp4NFnvPHeuD0kci6hvAQ2CpV71hf45FQTOHRVIN46%2BwhpU9MCPQ5Aj9eouNfkOUILiv79LRtIL%2BwzcZeXxvoCZazrBQLJrM7y8yBCLIZis11GyvI2whRgkUoRFW3zwXiTATTiFJjIdCp2PdOB0IYVHBFrC1WopDhi3h6ybD7CxRGHFYj8WJ2TkS2RldeAKPiglZRqj1E2PHE45NHA%2B4D3EYYvOZKNAFyEtOJUZglQJUEmUBK5DHGwp0VVfF7j6oeKQN4qe/jkTIGJApCcpoDDLUmKaVQ3swkRKifDPBcSVqUCSV5FJ20ZUsAOgQLJp1CkXUJgU3gRT7qPXvoayp%2BrqlfR%2BvUwGGwvKg3Bj0yG0MFAnpifqq1P8bX/3tcAkA0hAmTPze6mBcz4EcBhD%2BANFhNmXGmmYfYlwISumwBYOQloKMAC0KKXEtGCCjloABiFHprKAACogGtG4LjpG%2BMgAAErYGE0tZAwAdnFOcBAVwptjnoAeUbW5ctpZvPOXc%2BWILLbvOeXrHoymdafP1vps5wwTPaes40NT9twXBUc4m1tNDOBu1YSwBQa79jIgnA8HdIJqyYvwEIkt/KJGxQsQ8CxcX4sJYZTWpldcG5Nxbm3Llbm4Udsi/OxqIrJ6%2BPFQoCNeq9qyvXjUMryqD6VaPmujd/nxzbv0sF/dh7tU33Ky/Q1xrn4Go6G/C17b/7WokLaoZgCHU6H8M68BRHsOergQszgBHhxEdQU5yFmDw2SsjYjaNsaSEJsdtl2hv6hEHp9t53zzXAttcmDwyBhawvYpjri2dUUkvVzGXWhtZg1GSA0S26dvK9Ffai0Yh4Lda6TjSpIYHrNJCCUnUm9zo3IcOw4GYdHOXItLAegkvZkggA))
The following example stores the values into a compile, or runtime searchable map. We then use (randomly) get the string based representation using the enum value. The reverse is also possible (string search to enum value).
```cpp
#include <random>
#include <cstdio>
enum class foo {
  bar, tan, cos, sin, _BEGIN = bar, _END = sin,
};

constexpr auto foo_map = strtype::stringify_map<foo>();

int main() {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::array<foo, 4> foo_values{foo::bar, foo::tan, foo::cos, foo::sin };
  std::uniform_int_distribution<size_t> rnd(0, foo_values.size() - 1);
  for(auto i = 0; i < 32; ++i)
  {
    std::printf("%s\n", foo_map[foo_values[rnd(mt)]].data());
  }
  return 0;
}
```

# Licence

See the [LICENSE](LICENSE) file provided.