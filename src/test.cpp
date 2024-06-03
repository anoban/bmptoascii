// clang .\src\test.cpp -I.\include\ -std=c++ 20 -O3 -Wall -Wextra -pedantic -march=native

// #ifdef __TEST_BMPT_ASCII__

#include <iostream>

#include <bitmap.hpp>

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

[[maybe_unused]] static constexpr RGBQUAD min { 0x00, 0x00, 0x00, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD mid { 0x80, 0x80, 0x80, 0xFF };
[[maybe_unused]] static constexpr RGBQUAD max { 0xFF, 0xFF, 0xFF, 0xFF };

// a 300 byte chunk extracted from a real BMP file, for testing
[[maybe_unused]] static constexpr unsigned char const dummybmp[] {
    66, 77,  54, 129, 21, 0,   0,   0,  0,  0,   54,  0,  0,  0,   40, 0, 0,  0,   222, 2, 0,  0,   224, 1, 0,  0,   1, 0, 32, 0,   0, 0,
    0,  0,   0,  0,   0,  0,   196, 14, 0,  0,   196, 14, 0,  0,   0,  0, 0,  0,   0,   0, 0,  0,   2,   2, 8,  255, 2, 2, 8,  255, 1, 1,
    7,  255, 1,  1,   7,  255, 0,   0,  6,  255, 0,   1,  5,  255, 0,  1, 5,  255, 0,   1, 5,  255, 0,   1, 5,  255, 2, 1, 5,  255, 7, 4,
    6,  255, 8,  5,   7,  255, 7,   3,  8,  255, 8,   4,  9,  255, 8,  4, 9,  255, 8,   4, 9,  255, 7,   3, 8,  255, 6, 4, 8,  255, 5, 4,
    8,  255, 5,  4,   8,  255, 5,   4,  8,  255, 5,   4,  8,  255, 5,  3, 9,  255, 5,   3, 9,  255, 5,   3, 9,  255, 5, 3, 9,  255, 5, 3,
    9,  255, 5,  3,   9,  255, 5,   3,  9,  255, 5,   3,  9,  255, 5,  3, 9,  255, 5,   3, 9,  255, 5,   3, 9,  255, 5, 3, 9,  255, 5, 3,
    9,  255, 5,  3,   9,  255, 5,   3,  9,  255, 5,   3,  9,  255, 5,  3, 9,  255, 5,   3, 9,  255, 5,   3, 9,  255, 5, 3, 9,  255, 5, 3,
    9,  255, 4,  3,   13, 255, 7,   6,  16, 255, 8,   7,  17, 255, 8,  6, 18, 255, 8,   6, 18, 255, 7,   7, 19, 255, 7, 7, 19, 255, 6, 6,
    18, 255, 6,  6,   18, 255, 6,   6,  18, 255, 6,   6,  18, 255, 6,  6, 18, 255, 7,   7, 19, 255, 7,   7, 19, 255, 7, 7, 19, 255, 7, 7,
    19, 255, 8,  8,   20, 255, 8,   8,  20, 255, 8,   8
};

auto wmain() -> int {
#pragma region __TEST_BMP_STARTTAG__

    static_assert(bitmap::start_tag_be == 0x424D);
    static_assert(bitmap::start_tag_le == 0x4D42);
    assert(*reinterpret_cast<const unsigned short*>(dummybmp) == bitmap::start_tag_le);

#pragma endregion __TEST_BMP_STARTTAG__

#pragma region __TEST_TRANSFORMERS__

    constexpr auto ar_average { ::transformers::arithmetic_average {} };
    constexpr auto w_average { ::transformers::weighted_average {} };
    constexpr auto mm_average { ::transformers::minmax_average {} };
    constexpr auto lum_average { ::transformers::luminosity {} };

    static_assert(ar_average(min) == std::numeric_limits<unsigned char>::min());
    static_assert(ar_average(max) == std::numeric_limits<unsigned char>::max());
    static_assert(ar_average(mid) == 128);

    static_assert(w_average(min) == std::numeric_limits<unsigned char>::min());
    static_assert(w_average(max) == std::numeric_limits<unsigned char>::max());
    static_assert(w_average(mid) == 127);

    static_assert(mm_average(min) == std::numeric_limits<unsigned char>::min());
    static_assert(mm_average(max) == std::numeric_limits<unsigned char>::max());
    static_assert(mm_average(mid) == 128);

    static_assert(lum_average(min) == std::numeric_limits<unsigned char>::min());
    static_assert(lum_average(max) == std::numeric_limits<unsigned char>::max() - 1);
    static_assert(lum_average(mid) == 128);

#pragma endregion __TEST_TRANSFORMERS__

    constexpr auto ar_mapper { utilities::rgbmapper {} };

#pragma region __TEST_RGBMAPPER__

#pragma endregion __TEST_RGBMAPPER__

#pragma region __TEST_RACCITERATOR__
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr wchar_t wstr[] {
        L"A lookup that finds an injected-class-name can result in an ambiguity in certain cases (for example, if it is found in more than one base class). If all of the injected-class-names that are found refer to specializations of the same class template, and if the name is used as a template-name, the reference refers to the class template itself and not a specialization thereof, and is not ambiguous."
    };

    unsigned ncount {}, i {};
    for (::iterator::random_access_iterator it { wstr, __crt_countof(wstr) }, end { wstr, __crt_countof(wstr), __crt_countof(wstr) };
         it != end;
         ++it)
        ncount += static_cast<unsigned>(*it == wstr[i++]);

#pragma endregion __TEST_RACCITERATOR__

#pragma region __TEST_PARSERS__

    const auto fheader { bitmap::parsefileheader(dummybmp, std::size(dummybmp)) };
    const auto infoheader { bitmap::parseinfoheader(dummybmp, std::size(dummybmp)) };

#pragma endregion __TEST_PARSERS__

#pragma region __TEST_FAILS__ // regions for tests that will & must fail

#pragma endregion __TEST_FAILS__

    return EXIT_SUCCESS;
}

// #endif // __TEST_BMPT_ASCII__
