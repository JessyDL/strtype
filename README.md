# strenum
C++20 support for stringifying enums at compile time (with constraints) for *The Big Three* compilers (GCC, CLang, and MSVC). 

## How it works
(Ab)using the compiler injected macro/extension `__FUNCSIG__` or `__PRETTY_FUNCTION__`, at compile time parse the resulting string to extract the enum value, while rejecting entries that do not point to named values.

I don't take responsibity for when this breaks, compiler provided macros and extensions can always be changed, but as of writing (and looking at historical behaviour of these 2 values) they appear to be pretty stable. As of now none of the code goes explicitly against the standard, nor relies on UB (aside from the compiler specific macro/extension usage) and so I foresee this to keep working for some time.

## Limitations
The enum types are currently restricted to anything that satisfies `std::is_integral`, and `std::is_scoped_enum`. The integral limitation isn't really needed, it just lowers the potential oversight of unforeseen issues. Feel free to implement the `std::is_arithmetic` constraint, add tests, and make a PR if you're up for it!

Search depth: As we need to iterate over all potential values that are present in the enum, and as iterating over the entire 2^n bits of the underlying type is too heavy; we limit the search depth by default to `1024`, and offer 2 different iteration techniques (`strenum::sequential_searcher`, and `strenum::bitflag_searcher`). Both of these are tweakable, see the following section for info on how to do so.

CLang has a further limitation of how many times a fold expression can be expanded (256). We have a small workaround for this implemented in the `strenum::sequential_searcher` (one layer of indirection), but it means when the range exceeds 255 * 255 on CLang, the compilation will fail unless this limit is increased.

Duplicate named values (i.e. multiple enum names on the same `value`) will only fetch the first name it sees. So when the following enum is defined:
```cpp
enum class foo {
    bar,
    tan = bar,
    cos,
    sin,
};
```
The output will be an array of `{ "bar", "cos", "sin" }`, notice the missing `"tan"` (see this [on godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIAMzSrgAyeAyYAHI%2BAEaYxCBm0gAOqAqETgwe3r4ByanpAqHhUSyx8Ym2mPaOAkIETMQEWT5%2BgZXVGXUNBEWRMXEJ0gr1jc05bcPdvSVlgwCUtqhexMjsHOb%2BYcjeWADU5mYIBARJCiAA9OfETADuAHTAhAhe0V5KK7KMBHdoLOcAUpgFAoAJ4AEWC52GxEYPnOWAAbuctjtMFCCDCGHDobCWHcEEkkgcTBoAIIk8lmTYMbZePYmfxuZDDfCoBnYCkU3G7bZMYG7fiofYAdisZN2Et20QapApkt29QY%2B38YKlMrlkrQCll4slaQYOvJwrBDLF5LJzDYCiSTFWuxxWJYIrNEoImBYSQMboZbnZGuhXgcu1xAH0wvxiCwmDUGD7BeznRqJW8wsBdkJsKSAEpuAAS2CzytVDp8IBASgAjl4vngxCGlA1kAg4qak/b6o5kDyBMNMKoksRdkwvERdhZsABxACSEXlEpMooFqFQZelg4XJv8Lr1HbwXbQDF7/cHw9H2Aiqrn88XgrL%2BpFm%2B3Esu7eje%2B7h7dx/teAAXpgQwIXYAFlSQADRDDNszzSCpwALWwZ1djMDRJAADgfU1dhfVAkhjMRaBBXZUAROJiDwPYCGbe1MEbBAf3/UgFQQPAFF2ViGDADggPCTAsHQXYvAYeh%2BRBJZdhYoCqMwXZaDwFhCDuDUN1bMkVLUskwiAqMwggOZE11A8jwHIcRyFBExGrNiGWLDFcTvDFUzwKgQTjZd2T01TSQlCMICMoDTzZMwADZ20HEBdgs7wgX0hcnx3dAywHLSqAgYkzAAVgUEwMrcWMzDMJjoTudBoyYPS5i868TV1GECGWJUNC8jcOAWWhOAy3g/A4LRSBXDhfUsax7SWFYZI2HhSAITRWoWABrBJhTuNCzAATjQyQzC4fwMtW1bgq4Lh9E4SQupmvrOF4M4NCmmaFjgWAYEQFBUA9Og4nIShfiSd74mABRmFOBBUAIUhET3TAADU8EwG4AHkkkYThJpoWg3WIM4IGic7ojCBoQWR3hfjYQQ4eEgmet4LAoyMcRKbBvAYQcPBSLOem%2B0wZARzWSatKqc65Oia5iBBDwsEJqbyJYQmFioAx/uh2GEaR7heH4QQRDEdguEK9X5CUNRzt0I6DCMFBrGsfQ8GiM5IAWXCYzZgBaFkbNMIbLBQ3Ynbh/wrqqTmYxcBh3E8Fo9BCMI%2BlKAYjpSNIYzGPw4/yGNpn6eIjrsQPOhGJow5yLOA%2BZ2o8/TmPM9sPOk70SZGnL2YuAWBRRtWCQ2o6s76f63ZVDQ4KneCyRdmAZAuwgDEhLm/SIFwQgSH2Kkm94abKbmBZmyYLB4j00gFq4DLloOgrMskDQCuFSR9uOjhTtIaXgrMO4uA0Q7guFfwD64YUMqO7rev6ldEAN1V5aHuk9CASAOZcyIGQCgEAGj/WUIYKoQhgY3G6pNb6dA3wCGQeEWgaDUAYPOtg%2Bgf0AZMCBiDUgZC4hwxHEQkhqtSDQNJMQf6l1WGqE5nUfA3U1ayE1uIKQMhBCKBUOoemuh/D6EMMYC2lgrY23gPbPCGRnauxVO7Kwnteot2WG3LOBB%2BH4NQegzBK8YQ814Dca4SQZY306qQf%2BvB%2BrYB4TAhefcB5DxHmPXYE9iBTxnoNXRNhdhz1gYvT%2BcwV53Q3rRbelAO6314NLfwS00LrTQv4EKG0VpXxcedQBthgG3TXvNQYdxVpmA2v4fwGhH6JAaYVdqHA/bFO7lw0BrVSAPSgBApAdC4FfVej9chKBTbAEAsEhgc0waYARBDRW8NEaWL4HQdGmNsb01xswEWEtiZfDJoRc61N5F016vgJmjhWbnWgdzCWfN2m9UFsLUWGAbGS3ko4uWTAFYwzWSrFGQjRAiN1rICRhtpEBDkWbHRltBa213g7DRnAXYmLdooiwXsfadOziXPwEBXA1yOpHYoGc9DxwKJkAuydSA0rTlHGYsd2g51Lt0Ml7KiVdHriyqlxiuX0trmXAVFd26LEMdrVJzjXEXQ4L3fug9h68iMIEye8yZ5RIXhNOJFSwGJK3gMXeC1JD%2BDuBlSQOsMpWtWhlJpa1hQ3zvg/J%2BL834fy/j/P%2BJSuHXQNX0gZkCQCPNgZ9BBHCFBmMIRYo54ycExhjUw%2BVIypnyNmVPWhCbyEMOEHGlhbCo1cOgXwsIXC9bCO1tIPW0KpG9WNvChRHsbDItUX1dRPYMVaLBIivRV1W7ayrqYlBsbiEbLst8uxVDHHtLlX6jgHjOZmUHD4lVPJpkarmdPQJYTLaRPwNEvV8TKmkE3sk3e7TXUBCyTkvJwUCmJFWl0gB/rym9PXnvBIT8HVWsypk4UZhhRoQyv4NCN9OnytKR%2B/pEChkvTeuQiNaaETIEJCGBEXBVohi9ECAgIZVBD0Wcs1YqzlYbNRtsyguzer7PxvGlgJMCCnIptc90ly1jXMZjne57NPFPJYS8gW1sPli041Y35qtZbywUGR9ZEtK3gurWI/WkijYgGkNM82LblEorUY7Tg6JEp9txRobCcMzDYTBBebAFg5ATmwnBPA2EJxEXOBOAAYuZ5QAAVEAU43BefOHDXzIAszYCC2jZAwB/YdGcCSkO3KKXR0boy1OGRuVMoyA3NlhKYx8vztkBleXc5THFaluuhXw5Cv5ZSiVTcpVjUlXOrur7FWEeHiwBQqHIpYbuLh4YgSdXriXvqj9VSGl3AadNmbs2XXpJAI/Z%2Br8uDv0/r/H1L63FvpAXdWD8AQ1hpIBGxB0bR0poYxM3BDBk0Ft6ihtDSQMNYZw9GPDBGiMjLzRdwtnj2GcI4LwUtJjy2A5U1WiQNaoUG3rToBITbtPhN0%2B2tFXaOBGcR9YFCA7pWipHQQn7k1J0S2nQ4qTTjWvbcXfx6JHXdhdZ65h1a/W3uDYgHupRB754jdiSew183743uWne/J6En1bYVUA3bp6FqJDuB/R%2Be1NqSDF6hDKEHKeS8DZ%2B9pZhNfQYSaQUiGMMgaaAA%3D)). This is because during compilation `foo::tan` is considered an alias of `foo::bar`, and will be substituted. There is no way of recovering this information.

## Usage & customization points
The two entry functions into stringifying your enums are `strenum::stringify<YOUR_ENUM_TYPE/VALUE>()` and `strenum::stringify_map<YOUR_ENUM_TYPE>()`.
The first function will return you an `std::array<std::string_view>` if given an enum type, otherwise when given an enum value it will return you a `std::string_view` representation of the enum value. In the case of the array return the values are sorted based on the underlying enum values.
The `stringify_map` function will return you a compile and runtime searchable associative container where you can search for the enum value based on its string representation and vice-versa.

Your enums should either come with a `_BEGIN`/`_END` sentinel values in the enum declaration, or you should specialize the `strenum::enum_information` customization point (see example section). Note that both the specialized `END` and the embedded `_END` act as **inclusive limits to the range**. This means unlike normal ranges, which are exclusive ranges, the endpoint is used as the last value. This is the mathematical difference of `[0,10]` (range of 0 to 10, inclusive) and `[0,10)` (a range of 0 to 9, excluding 10). This was done for convenience so that users don't need to define `END` as `END = some_value + 1`. This is *only* the case when within the enum declaration scope, or when `END` is set as an instance of the enum type object; if it's set as its underlying type then it behaves like an exclusive range limitter again.

By default the search iterations is limited to `1024`, this means if the difference between the first and last enum value is larger than that, you'll either have to specialize `strenum::enum_information` for your type, or globally override the default value by defining `STRENUM_MAX_SEARCH_SIZE` with a higher value.

Lastly the search pattern. There are 2 provided search patterns `strenum::sequential_searcher` and `strenum::bitflag_searcher`. Both will search from `_BEGIN` to `_END`, but have a different approach.
- `sequential_searcher`: iterates over the range by adding the lowest integral increment for the underlying type from `BEGIN` to `END`.
- `bitflag_searcher`: iterates over the range by jumping per bit value instead (so an 8bit type will have 8 iterations, one for every bit + the 0 value). Combinatorial values are not searched for. For example if there is a value at 0x3, which would be both first and second bit set, it would be skipped.

You can provide your own searcher, as long as it satisfies the following API:
```cpp
struct custom_searcher {
  template<typename T, auto Begin, auto End>
  consteval auto max_size() const noexcept -> size_t { /* return the theorethical max value for your enum type */ }

  template<typename T, auto Begin, auto End, auto GetEnumName /* optimized version of stringifying enums*/>
  consteval auto operator() const noexcept -> std::array<std::string_view, /* size must be calculated internally */>
};
```
See `strenum::sequential_searcher`, or `strenum::bitflag_searcher` for example implementations. Note that at least the `sequential_searcher` has some compiler specific performance optimizations and workarounds which do complicate the code a bit.

## Examples

### compile time stringify an enum ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjE/lykAA6oCoRODB7evv5JKWkCIWGRLDFxZgl2mA7pQgRMxASZPn4BldUCtfUEhRHRsfG2dQ1N2a1D3aG9Jf3lAJS2qF7EyOwc5gDMocjeWADU5mYIBASJCiAA9OfETADuAHTAhAheUV5Ky7KMBHdoLOcAUpgFAoAJ4AESC5wUBGIjB85ywADdzlsdpgoTC4X9obCGD47ghEokDiYNABBUlkrG7bZMYG7fiofYAdis5N2HN2UXqpF2dQYvLQCl5qQFlJMzLBJnWbIp5OYbAUiSYK12OOpEtlHIImBYiQMOulbml2EpHJxXgcuyxAH1QvxiCwmI4BEbGSaWVrOW9QsBdkJsGSAEpuAAS2CD%2B3WYLVmLxLBAICUAEcvF88GIbUp6sgELFpV7zXVHMgaQJoZhVIliLsmF4iLsLNgAOIASXCnM5moZqFQie5NYlUplZs50OdeFLaAYFarNbrDew4Rjna7rJ7faToRZw8Lu0usYnU/LOrnarwAC9MDaCLsALJkgAaNoDwbDL9bAC1sJ7dmYNJIAAcO4Fvu5y7KgiQuswtC0CCEGIrExB4HsBB5mqmA5gg55XryaF4AouwEQwYAcLeYSYFg6C7F4DD0PSIKLLsCCEHy6G0HgLCEHco5DgW4qSuK5KhLeTqhBAsyeqO06ztWtb1kyiJiGmhHSjG6rxomOK%2BngVAgm6vYmhJ/HsoeJY2nS7wEBASneECJgAKwWBojkxmpan7GYZgDgcswmWSHKwgQSwMLsLkjuSQ4cPMtCcA5vB%2BBwWikH2HDGpY1hqosyyYJ56w8KQBCaNF8wANb%2BMydyAWYACcgGSOU6wOTVNUAGxcAksUcJICXFSlnC8GcGiFcV8xwLAMCICgqB6nQsTkJQvyJHNcTAAozCnAgqAEKQSKTpgABqeCYDcADyiSMJwBU0LQOrEGcEBRH1UShPUIJXbwvxsIIp10e9SW8FgTpGOIAO7XgsLVIhZxg5WVT1qsBUiZgXXJRxUTXMQIIeFgH2FchCbcNFfAGGtR0nedl1EzIggiGI7BcAE/CCIoKjqGDuidYYxjWNY%2Bh4FEZyQPMkHQTDAC00LoGppgZZY/67OLp3rINKNVNBLgMO4njNHowSTMUpR6MkqTQSMfgJCb%2BQMD0hv9BUavtAwnTDDr2QO/Y0EuxMRR9HEFTjObejjg0tt%2BxI8wKNlKwR/ocW9WDqW7KogGteLrWSLswDIKWEAwrRpWSRAuCECQeVcLMvBFQDszzHmTBYHEEmkOVXAOVV7VeWYDmSBoXnMpIbVx91vAJq1Zh3FwGgda1zL5Q5XDMgvpCJclqWDSAw3V1oY2TRASBw8gClkBQED1GtyiGCjQhbTciUFUtdATgIl9hLQN%2BoHffWP/Qq3rUwm1tqkB/rEU69YP5f2pofMkxA1oDUCKoKotR8CJV4MzYQohxBSBpvIJQag%2Bq6HWPobmKBeaWH5oLeAIsoLpAllLGWZCLD/kGtHBmgwUGv2vrfe%2BVdYSI14Dca4iQPoxXjivPqqVsCIKPkQGsKc04ZyzjnXYediAFyLulKw5Ddgl1keXSuI0a510wo3SgoiR6kATOsSqgE6qAXWGYVq9VqqD3EYneBQ1DE7zKv4SQdwapmHqusdYGhx5mEkMEgIXUVZuLXvA7exNxpQD3kgEBJ9FozWWr/FABgjA3jUQwUqu1MCIn2uTM6F0eF8DoHdB6T0wYvWYFjPGX0vi/Tgn1IG3NQbJXwJDRw0M%2BqHwRnjZGqNeDo0xtjDA/D8acRESTJgZNjoVKptdWQdMsFM1kKzfBHMQBENyTzOWNh0ZC2bqLWhnBJYEGltGWWWimEaEVsrVWnt0ia21lkC2gQtZh2mP7XIpt0hB0tnkaC/yjYe3VjUQObsfltC9uMSF9tBhdFBWi0OBtw4VwWEsGOuKurxVibwJO8j06Z1pEYFR%2BdClF10WXDYFcq6jWMQ3fozdyoRLuD3RmDke41QcqE2qzJh49UsSAcek9p5cFnvPRey9V6ko8ZvLxiSUkgGGbIhaZ9YEKE4e/bhLTMlP2ggaiBSrgEmuyVS4A%2BSC5Wtmr/MBwgjVQOkTAuBHBeCH2QaEeB6DNkM2kOg3Z7Nkqc2IUYUhJyKHnOoWLa59D7mMOYXinKwdbmhHNW6gqmJZmCIAQsolCc4kcCkfDPR5LFG2ppQUwuKjNF8x0fgPRTKDEJNrqQeupjm5dXFVYmxdiHFOKAuEmqJL%2BretsKqztPivI8o0D3bu1jmRmGZIBBy6xALDxiZa9eard7wCSfvaaTr5qnzSSAREyAiQ2kRFwGqNoDRAgIDaVQGdimlJWOUymVSbq1MoPU5KjS3rGpYN9Ag7T/q9N1N01YvSIYwsGbDaRIzqZjL6pMt6OMEO8PmUTeYVBSYKF/ZUvGgbMHBpwWGghIBpBHJjY8uNVCUo0PLJwDE6AmPWAVucU6Zh9xgmXNgCwchmz7k/HgfczZ4LnGbAAMX3KdZQAAVEArY3BKf42pkAQZsDadusgYAbyYXOAgK4DF%2BtfYAuNuCkF8K7PAoKNi2z0KnbewxYi2FXQUWApDo0RzAdfOuahZHVhscS2TrJYolgCgb27AfTVO4L7oQqIZYOMw%2BUO2spbgc9YdxglFeKyVsVo9JUTynjPOebcFUJH3Sqreo1SAnoPmh7Vp9z76qvoaz%2BVSf7PwYDmvr39rX9BvXepLz7nSvvfZ%2BtJLqLV42gXq%2BBvqs2oJwUGiQIadl4PDToHIjGHl8zOaxy5HGOBcZ4/LZKUd8VsKltmnrS3qb5rxoW4RhHh7Eoa%2BW9rZcP2ZziwlpLKWZtpYgE27RGX9EsqMWViV1iqrDscc48d0XGuHvnX4ue48WoNUkGOgCDld2luVdOudw8zDk6nfD7xpBEL3Q%2BZIIAA%3D%3D))
Following example showcases an enum being stringified, and accessible as a contiguous array of `std::string_view`'s.
```cpp
enum class foo {
  bar, tan, cos, sin,
};

namespace strenum {
  template<>
  struct enum_information<foo> {
    using SEARCHER = strenum::sequential_searcher;
    static constexpr auto BEGIN      { foo::bar };
    static constexpr auto END        { foo::sin };
    // static constexpr size_t MAX_SEARCH_SIZE { 2048 }; // optionally override the search size, this isn't needed unless you hit the limit.
  };
}

int main() {
  constexpr auto values = strenum::stringify<foo>();
  static_assert(values[0] == "bar");
  return 0;
}
```

### compile time stringify a single enum value ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGIM6SuADJ4DJgAcj4ARpjEElykAA6oCoRODB7evv5JKWkCIWGRLDFxXAl2mA7pQgRMxASZPn4BldUCtfUEhRHRsfG2dQ1N2a1D3aG9Jf3lAJS2qF7EyOwc5gDMocjeWADU5mYIBASJCiAA9OfETADuAHTAhAheUV5Ky7KMBHdoLOcAUpgFAoAJ4AESC5wUBGIjB85ywADdzlsdpgoTC4X9obCGD47ghEokDiYNABBUlkrG7bZMYG7fiofYAdis5N2uyi9VIuzqDB5aAUPNS/MpJmZYJM6zZFPJoQIuxYTFCEFmLJlHLQDGhmFUiWIuyYXiIDNQqC5xAA%2Bjj9uswbscViQCAcaFHlQQVK3IznRapdhVVKNQ66o5kJa6e8CNQzRbrTDbZK7SH0M7XUZLYi8Jgbur9mYzH6CyywbMg5SObCCEsGLsNOXyeKwRx5rROABWXh%2BDhaUioThuazWB2LZaYfPrHikAiaFvzADW/mZdwAHGYAJwryRmLjrdvr9cANnK%2Bk4ki7s77nF4Zw009n8zgsBgiBQqBYiTosXIlF%2Bn/ocTAAozCnAgqAEKQSJ4CsABq2Y3AA8okjCcFONC0AQsRnBAUSXlEoT1CCqG8L8bCCAhDC0ERPa8FgSpGOINGQXgsLVIiQKXrqVTGqsU7ypgbZMbQeBRNcxAgh4WDEdOxB4CwxHzFQBhAXBOZISh3C8PwggiGI7BcAE2nyEoaiXroCQGEYKBDpY%2BgiWckDzKgiSOAIZwcAAtNC6BSmCpiWNYZgaLsHkIesN4CVUrl%2BBArgjH4CTBJMxSlHoySpNF8VpXk0U9ClMy2JF7QMJ0wyeM0ehtNFpUTEUfRlIMXRZRU4x5fVEjzAoo4rB1p4cJ2pDdr2/YcLsqgroeHmHpIuzAMgyC7BAMJeAw85qhAuCECQE5cLMvAzjRszzAgmBMFgcSqqQi5cO2q7HgWZjtpIGgFsykhHn156kPJh5mHcXAaOUh7MpO7ZcMyYODZeI03iAd4HVoj4vhASBccgxokD%2BED1EByiGAJQhgTc3ZTn%2BdBMNFeNhLQhOoMTl5kwBIBASBChgRBjOxAhxq0/TmmBKoVRksQQHXgLVS1Pg3ZabIuniFIMiCIoKjqExujrPohjGDZNjCVEDmXc50XuV5BA%2BXa/lWJYQU3t1%2BmDFLVME0TJP7bCvG8Dc1yJApfUDUNvAjdggvo0QBrjZN02zfNi3Lat62DgFtm7JtYc7Xt96Hcdp3nZQrZnrw8nrMuK6biu6xmIeW5ru9UNMTDthw5niMLv4kh3OuZhbus6waL9ZiSD3ASCeFdfDWLCMtqQT5QMjSCc2QFAQAvKCWcAlpx/OkGYFmsHweprt8HQmHENhuFMfhzDidJpFfBRVGXnRWuMb2%2BCsY47Hub2aM8dJ/GCb2PWYkJIYA9jJOSvslJMBUvvZCh8jJy30oZWQytTJqxABrNe1kk663svAJyLl0gm28r5S2gVgqhVHlVdILgGDuHKtkRKdC2rTAaulfIGQGEJVyBldILDUoVCKtVcYzVCr2GEV0fhBVoRNS4ZVVqyV2q7QWEsHqyjBL%2B2hpwMaE0pozVpEYWOxAVprUWqnbaGxdr7QfNnM6/RLqLkHncJ6Bl2xPXXO2PuG5mSfULiAX6/1AZcGBqDcGkMA5Xg4LDeGD5p5zxAD/MOWMcYKCdjTF2N93z/gpukNJvMIkrwMevTepAF7c2EBk/maNhaiyieLZAktQhiwQaIeW0gjKoNVr2cymsrJkNsnrA2BDjacFNubPyOsbYqLHPIx2%2BN0l00PpiMBXsmA%2B00vnfqF566cGDtxNOEc9E0jXkYkxCcdY8nMQaSxGdJ5HVICdOxF1NlfSLiXMuFcq6SBruuMegcxa3mbnOK6/g/qeKeo9YuzIzDMhXO2dYK4%2BqjwiQ3O5cT4AzxRm%2BD8X5F6/iybikAiJkBEkzFwdcloDCYWhJaVQ01t670wKpRCcDpLoRPmfPCBFr781vuRSi1FX6YHosAF%2BtEWJRTwJ/TiIdf783/peIBhFJKrF7DCCBGy%2BDKQUMyg%2BbLZatP0u0lBJkuk6BANILB/TcH63wX2QhblOAYlTNaoKuxzgITMO6sE4QwTYAsHIAA4u6gAWngd1gaQQRoAGLuoQsoAAKiAAAkm4WNHrE0gAAErYHTRhZAwAIriJobFOhoikp1VYdlXhAhRHsNyooqtgji0dBEXI5tkrW2SMbQIxqZUsjcJkQ0KRZROp216ho7Z49Rp0pmiwBQxLdiInJXcKlQIFQbXwGnG51is4gp7ncHuR7j0nt8d9fxf0AZAxBjdMJCQUUAqbmizFqNZVJKXikvJlTSYEqpbk%2BZ%2BSGa/v6MS0ly6KVrppbO0pwHiDlMA1UkONT3K8DRo06WithCGokMapWpqzI5CtRcwZdqjZEKdd5bBVsLBTK6qo%2B23lQhfsWdJZZ0lVnrJ4JszROyOB7NDttWdioF0LXA6uim67FqJ2o5crdFizCTluTYs9bzVwfMrtXAevyH11MBXc1uA87gg1%2BgebckhvlBUkO2JFU7/l1P031MwtnIm7pbqQdip8aGSCAA%3D%3D%3D))
Here we stringify only a single value of the enum, note the abscence of the customization point as it's unneeded.
The return type of this function is a unique type that holds the string as a NTTP.
```cpp
enum class foo {
  bar, tan, cos, sin,
};

int main() {
  // note: the return type of foobar_str is a type containing the string as NTTP argument.
  constexpr auto foobar_str = strenum::stringify<foo::bar>();
  static_assert(foobar_str == std::string_view { "bar" });
  return 0;
}
```

### runtime stringify enum values ([godbolt](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxHoADqgKhE4MHt6%2BcQlJAkEh4SxRMVy2mPaOAkIETMQEqT5%2BRXaYDskVVQQ5YZHRegqV1bXpDX3twZ353VwAlLaoXsTI7BzmAMzByN5YANTmZggEBLEKIAD0x8RMAO4AdMCECF4RXkrzsowEV2gsxwBSmAoKAE8ACIBY69YiMHzHLAAN2Oaw2mDBBAhDCh4MhLCuCFisR2Jg0AEECcSzKsGOsvFsTMs3OcGOhUCwadgSSsEVTMNtachevhUCySZjNusmP9NvxUNsAOxWImbTYRKqkTaVBgqtAKFWJdWbAD6FmwAHEAJKhblAxXK/XYUKWmmWnWkNnSoE0uXEoloBi9TCqWLETZMLxECWoVB6lhMWIWzYYtEsEAgcHBW5UAGR6M0tySlkQSbutlE4IETZR4L5mUehV8pP0xksPWwvALTbEdCF%2BVxgjoJMsAhcACcg%2BW0rLBAg7fzBeW1e7vZAVXOAOzkpVkhZYYjMLEXj%2BJllkqTSrIW6TapVR5QCUv4aTOplbtnJJrPaTXgYeH4xEbJabeBTR4ygYbNEgAL0wPUCE3YgGQgDRb23Xc/iucDMErABaTYpk7QkFW/CBg1DPBYw0d1NhI7NNmWMxyOsaw8ALLsDznV8FwDEsqAgfEzAAVgUExeLcECzDMRDMzxXiLElPUd28fcpNg9AIH7AteLddSrnQJhKmnXCFQPN0uwhAg5gYTYyOfIlDI4aZaE4XjeD8DgtFIVBODcejLDjWZ5i5FYeFIAhNFs6YAGsQDMaUrgADjMQcYskMwuGWXjhwANi4Ip7I4SQnJCtzOF4I4EOClzbNIOBYBgRBrxYWI6GichKE%2BBr6BiYAFGYQ4EFQAhSGbBYADU8EwC4AHlYkYThApoWgCGiI4IAiAqImCKoARm3hPjYQRxoYWhNvKgbMCjIxxGO/AIWaGE/gKv0mhDRZApLEoCtoPAInOYgAQ8LAtqC4g8ETbgKqoAxOpGsbJum0GZEEEQxHYLgxP4QRFBUdRjt0bLDGMLybA%2BiIjkgaZUFiYCjg4DC%2BQdUxLGsMwNE2DDxuWYqSiaYCXAZAZ6lIQJRjyAoMkSYC%2BdFrIGA6YWJmKUoWmGCWGk55pymGGWukKWwlc8Ooeg1oWtYkaYFF8hYTf0Bz8uO9yOE2VQYvSjD0skTZgGQZBNggFEPzCyZvdwQgSG2Mkpl4MqtEmaYEEwJgsBifNSAirheNizLRL4yQNFE6VJEHdKrdy3hE3Sswri4DQsvS6VllTrhpV4opnNcu3ipAUqQumKraoe5AQxIZrCOITrlEMEohF6i5nMC1q6B05Ix5CWhJ9QaeCrn9qQE67qFF6/rN%2BicaQ1X9fjr7wkR6p3g%2B4qfBnN4NHhFEcQpHh%2BQlDUArdGWfQ8ZQAm%2BhPokyTuTSmnAaY9jpgTJmxVzbIx1vfJeE8p4zwjhCZ6vALjnFiFtOy1tSAt14HbbAqhHpEEDI7Z2rt3ae29r7Bg/tvaeQZpYFUQcKGhzrpMCOXcY5xwTpQfBxdSCJlHLFBKMUaLpUSnFfOhCCpt1sB3IKfDk6RUkFcQcZhErLGWBoMuZhJB6LEjldmCjbZFVUeVbuNUe4QCQIfMgFAIBOJQAYIwUFiB%2BxOjCFsmAoYTSmmgvgdAFrECWitY6a1mA/QBjtN4%2B1DoFSwGdYAF1XJXS5ngW6VNXJ9yegDV6OVXJE2%2Br9DAmDAbAzwXwCGChAkwxCU/RGr9UayAxl/bGIBf4ePxqwwmwD4BkwpskKmkCOzLCBPTKwlgmYszZhzBWzgICuGVgLBkmtxja3iGLZI6zdlSy2SLFWyyGCtH6HrQY8tsnqzaMcuWvQ2jrKedUB5hRTbwMtjlRyFjW6cAdk7F2btRRGHod4xhAcIAcJDgFHh1io78Pjt0JOEVjFXF4pIFGvFMWDl4gY%2BK0oi55VESAMuFcq5cBrnXJujdm6KKsSVBFFV7FIAKRQoeVRR7jxXqg%2BJTI2oLwEMg3la8iGkDcaC4AXifFOOPsIPlcML5XysbfHswQrEtJfsjaQT9OlY1cjjP%2BRgAEDKAcTYZblRkCHGbTKZMzGauTNnMC2QwkE8tPiElEmAqnYOjLUn5Nt/kcFIeQkOVDgUij6eCv2UKWGzJsJsGFgY4W8JsUiwRSccokrEdFGKkjpGyKMYOP5xDGUqMjqFdRokMUaExXxUc0oooxV4ssGKRdzHiqUZW2x8BWV1Tak1FxbiYTIFxLJIceoDALV6HqVQrtfH%2BMacEgGc1wmRNWutOJcMEl7QOkdTJp08YZN4Fkm6d1z5kP7gtIpgg3rHTKRtP6ixXIohqaDaY4MmCQ1GkE2Gs1ZCtJ1e/fV38QDSD6aahN5qQEjPARwZEvYHVzOZsccaZhNjHCBHabAFg5BGkwwALRIscI0AJMNGgAGKYfGsoAAKiAE0bhqNofoyAAAStgFj81kDACWbcvwqzeZXP5oLXIxsiiHPFiJyWwF3k9FVsBC5NQZOnIE8p%2BTQxnmqZ1vco22zLbOr8t8ghXaAXzrdiwBQo7NgwiHFcadfxSzQvwJw1NzLo7qL0VcPRvm/P%2BeJSXMl5dK7V1rvXOlpbCocHbp3GxlU7FsqvQPZxlAuUKBFZ6/l9V57AUy4q1yI6x2xAnYOKdOknNzoXXKk%2BirArKs6qqq9d8NUxffkBiQuqOmfwNToSKxr%2BnQaJrBq18HENQcdXAl1CC%2BTBHy2KgG3rfU4IDaZhlIbkucIs2WazXs7ODgcxV3ozDAFJtc7CsO8Ke3CJzT0vNBazAyMkHIktZm2tMuu9WzRtcy7DiSpIZ7TNJC8Q7UGstbXPs5TMGD6LabEWkFuhE5I4GgA%3D%3D%3D))
The following example stores the values into a compile, or runtime searchable map. We then use (randomly) get the string based representation using the enum value. The reverse is also possible (string search to enum value).
```cpp
#include <random>
#include <cstdio>
enum class foo {
  bar, tan, cos, sin, _BEGIN = bar, _END = sin,
};

constexpr auto foo_map = strenum::stringify_map<foo>();

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