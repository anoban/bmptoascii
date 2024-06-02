// #ifdef __TEST_BMPT_ASCII__

#include <bmp.hpp>

// should work with transformers if T is unsigned char
template<typename T, typename = std::enable_if<std::is_arithmetic_v<T>, bool>::type> struct RGBPIXEL {
        T rgbRed {};
        T rgbGreen {};
        T rgbBlue {};
};

// even though the types of members are right, the names aren't compliant with our requirements, shouldn't work!
struct PIXEL {
        unsigned char Red {}, Green {}, Blue {};
};

// should qualify for the use with transformer objects as this satisfies both our constraints!
struct OBJECT {
        unsigned char      rgbRed {};
        wchar_t            _dummy0[20] {}; // NOLINT(modernize-avoid-c-arrays)
        unsigned char      rgbGreen {};
        unsigned char      rgbBlue {};
        float              _dummy1 {};
        unsigned long long _dummy2 {};
};

[[maybe_unused]] static constexpr RGBTRIPLE rt_min {};
[[maybe_unused]] static constexpr RGBTRIPLE rt_max { 0xFF, 0xFF, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD   rq_min { 0x00, 0x00, 0x00, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD   rq_max { 0xFF, 0xFF, 0xFF, 0xFF };

auto wmain() -> int {
#pragma region __TEST_BMP_STARTTAG__
    static_assert(bmp::start_tag_be == 0x424D);
    static_assert(bmp::start_tag_le == 0x4D42);
#pragma endregion __TEST_BMP_STARTTAG__

#pragma region __TEST_TRANSFORMERS__

    constexpr auto ar_average_rq { ::transformers::arithmetic_average<> {} };
    constexpr auto ar_average_rt { ::transformers::arithmetic_average<RGBTRIPLE> {} };
    constexpr auto ar_average_rp { ::transformers::arithmetic_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto ar_average_ro { ::transformers::arithmetic_average<OBJECT> {} };

    static_assert(!ar_average_rq(rq_min));
    static_assert(ar_average_rq(rq_max) == UCHAR_MAX);
    static_assert(!ar_average_rt(rt_min));
    static_assert(ar_average_rt(rt_max) == UCHAR_MAX);

    constexpr auto w_average_rq { ::transformers::weighted_average<> {} };
    constexpr auto w_average_rt { ::transformers::weighted_average<RGBTRIPLE> {} };
    constexpr auto w_average_rp { ::transformers::weighted_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto w_average_ro { ::transformers::weighted_average<OBJECT> {} };

    constexpr auto mm_average_rq { ::transformers::minmax_average<> {} };
    constexpr auto mm_average_rt { ::transformers::minmax_average<RGBTRIPLE> {} };
    constexpr auto mm_average_rp { ::transformers::minmax_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto mm_average_ro { ::transformers::minmax_average<OBJECT> {} };

    constexpr auto lum_average_rq { ::transformers::luminosity<> {} };
    constexpr auto lum_average_rt { ::transformers::luminosity<RGBTRIPLE> {} };
    constexpr auto lum_average_rp { ::transformers::luminosity<RGBPIXEL<unsigned char>> {} };
    constexpr auto lum_average_ro { ::transformers::luminosity<OBJECT> {} };

#pragma endregion __TEST_TRANSFORMERS__

#pragma region __TEST_RGBMAPPER__

#pragma endregion __TEST_RGBMAPPER__

#pragma region __TEST_RACCITERATOR__
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr wchar_t EULA[] {
        L"Click on the green buttons that describe your target platform. Only supported platforms will be shown. By downloading and using the software, you agree to fully comply with the terms and conditions of the CUDA EULA"
    };

    constexpr auto begin { ::iterator::random_access_iterator(EULA, __crt_countof(EULA)) };
    constexpr auto copy { begin };

    for (auto it = ::iterator::random_access_iterator(EULA, __crt_countof(EULA));
         it != ::iterator::random_access_iterator<wchar_t> { EULA, __crt_countof(EULA), __crt_countof(EULA) };
         ++it);

#pragma endregion __TEST_RACCITERATOR__

#pragma region __TEST_FAILS__ // regions for tests that will & must fail

#pragma endregion __TEST_FAILS__
    return EXIT_SUCCESS;
}

// #endif // __TEST_BMPT_ASCII__
