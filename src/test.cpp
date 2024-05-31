// #ifdef __TEST_BMPT_ASCII__

#include <bmp.hpp>
#include <utilities.hpp>

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

[[maybe_unused]] static constexpr auto rt_min {
    RGBTRIPLE { 0x00, 0x00, 0x00 }
};

[[maybe_unused]] static constexpr auto rt_max {
    RGBTRIPLE { 0xFF, 0xFF, 0xFF }
};

[[maybe_unused]] static constexpr auto rq_min {
    RGBQUAD { 0x00, 0x00, 0x00, 0xFF }
};

[[maybe_unused]] static constexpr auto rq_max {
    RGBQUAD { 0xFF, 0xFF, 0xFF, 0xFF }
};

auto wmain() -> int {
#pragma region __TEST_TRANSFORMERS__

    constexpr auto ar_average_rq { utilities::transformers::arithmetic_average<> {} };
    constexpr auto ar_average_rt { utilities::transformers::arithmetic_average<RGBTRIPLE> {} };
    constexpr auto ar_average_rp { utilities::transformers::arithmetic_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto ar_average_ro { utilities::transformers::arithmetic_average<OBJECT> {} };

    constexpr auto w_average_rq { utilities::transformers::weighted_average<> {} };
    constexpr auto w_average_rt { utilities::transformers::weighted_average<RGBTRIPLE> {} };
    constexpr auto w_average_rp { utilities::transformers::weighted_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto w_average_ro { utilities::transformers::weighted_average<OBJECT> {} };

    constexpr auto mm_average_rq { utilities::transformers::minmax_average<> {} };
    constexpr auto mm_average_rt { utilities::transformers::minmax_average<RGBTRIPLE> {} };
    constexpr auto mm_average_rp { utilities::transformers::minmax_average<RGBPIXEL<unsigned char>> {} };
    constexpr auto mm_average_ro { utilities::transformers::minmax_average<OBJECT> {} };

    constexpr auto lum_average_rq { utilities::transformers::luminosity<> {} };
    constexpr auto lum_average_rt { utilities::transformers::luminosity<RGBTRIPLE> {} };
    constexpr auto lum_average_rp { utilities::transformers::luminosity<RGBPIXEL<unsigned char>> {} };
    constexpr auto lum_average_ro { utilities::transformers::luminosity<OBJECT> {} };

#pragma endregion __TEST_TRANSFORMERS__

#pragma region __TEST_RGBMAPPER__

#pragma endregion __TEST_RGBMAPPER__

    return EXIT_SUCCESS;
}

// #endif // __TEST_BMPT_ASCII__
