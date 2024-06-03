// #ifdef __TEST_BMPT_ASCII__

#include <bmp.hpp>

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

[[maybe_unused]] static constexpr RGBQUAD rq_min { 0x00, 0x00, 0x00, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD rq_mid { 0x80, 0x80, 0x80, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD rq_max { 0xFF, 0xFF, 0xFF, 0xFF };

auto wmain() -> int {
#pragma region __TEST_BMP_STARTTAG__

    static_assert(bmp::start_tag_be == 0x424D);
    static_assert(bmp::start_tag_le == 0x4D42);

#pragma endregion __TEST_BMP_STARTTAG__

#pragma region __TEST_TRANSFORMERS__

    constexpr auto ar_average { ::transformers::arithmetic_average {} };
    constexpr auto w_average { ::transformers::weighted_average {} };
    constexpr auto mm_average { ::transformers::minmax_average {} };
    constexpr auto lum_average { ::transformers::luminosity {} };

    static_assert(ar_average(rq_min) == std::numeric_limits<unsigned char>::min());
    static_assert(ar_average(rq_max) == std::numeric_limits<unsigned char>::max());
    static_assert(ar_average(rq_mid) == 128);

    static_assert(w_average(rq_min) == std::numeric_limits<unsigned char>::min());
    static_assert(w_average(rq_max) == std::numeric_limits<unsigned char>::max());
    static_assert(w_average(rq_mid) == 127);

    static_assert(mm_average(rq_min) == std::numeric_limits<unsigned char>::min());
    static_assert(mm_average(rq_max) == std::numeric_limits<unsigned char>::max());
    static_assert(mm_average(rq_mid) == 128);

    static_assert(lum_average(rq_min) == std::numeric_limits<unsigned char>::min());
    static_assert(lum_average(rq_max) == std::numeric_limits<unsigned char>::max() - 1);
    static_assert(lum_average(rq_mid) == 128);

#pragma endregion __TEST_TRANSFORMERS__

    constexpr auto ar_mapper { utilities::rgbmapper {} };

#pragma region __TEST_RGBMAPPER__

#pragma endregion __TEST_RGBMAPPER__

#pragma region __TEST_RACCITERATOR__
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr wchar_t EULA[] {
        L"Click on the green buttons that describe your target platform. Only supported platforms will be shown. By downloading and using the software, you agree to fully comply with the terms and conditions of the CUDA EULA"
    };

    constexpr auto begin { ::iterator::random_access_iterator(EULA, __crt_countof(EULA)) };
    constexpr auto copy { begin };

    unsigned i {};
    for (::iterator::random_access_iterator it { EULA, __crt_countof(EULA) }, end { EULA, __crt_countof(EULA), __crt_countof(EULA) };
         it != end;
         ++it) {
        assert(EULA[i] == *it);
    }

#pragma endregion __TEST_RACCITERATOR__

#pragma region __TEST_FAILS__ // regions for tests that will & must fail

#pragma endregion __TEST_FAILS__

    return EXIT_SUCCESS;
}

// #endif // __TEST_BMPT_ASCII__
