// clang .\src\test.c -I.\include\ -std=c23 -O3 -Wall -Wextra -pedantic -march=native

// #ifdef __TEST_BMPT_ASCII__

#include <bitmap.h>

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

static const RGBQUAD min { 0x00, 0x00, 0x00, 0xFF };
static const RGBQUAD mid { 0x80, 0x80, 0x80, 0xFF };
static const RGBQUAD max { 0xFF, 0xFF, 0xFF, 0xFF };

// a 300 byte chunk extracted from a real BMP file, for testing
static const unsigned char const dummybmp[] = {
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

int            wmain(void) {
#pragma region __TEST_BMP_STARTTAG__

    assert(start_tag_be == 0x424D);
    assert(start_tag_le == 0x4D42);
    assert(*(unsigned short*) (dummybmp) == start_tag_le);

#pragma endregion __TEST_BMP_STARTTAG__

#pragma region __TEST_TRANSFORMERS__

    assert(arithmetic_average(&min) == 0);
    assert(arithmetic_average(&max) == UCHAR_MAX);
    assert(arithmetic_average(&mid) == 128);

    assert(weighted_average(&min) == 0);
    assert(weighted_average(&max) == UCHAR_MAX);
    assert(weighted_average(&mid) == 127);

    assert(minmax_average(&min) == 0);
    assert(minmax_average(&max) == UCHAR_MAX);
    assert(minmax_average(&mid) == 128);

    assert(luminosity(&min) == 0);
    assert(luminosity(&max) == UCHAR_MAX - 1);
    assert(luminosity(&mid) == 128);

#pragma endregion __TEST_TRANSFORMERS__

    constexpr auto ar_mapper { utilities::rgbmapper {} };

#pragma region __TEST_RGBMAPPER__

#pragma endregion __TEST_RGBMAPPER__

#pragma region __TEST_PARSERS__

    const auto fheader { bitmap::parsefileheader(dummybmp, std::size(dummybmp)) };
    const auto infoheader { bitmap::parseinfoheader(dummybmp, std::size(dummybmp)) };

#pragma endregion __TEST_PARSERS__

#pragma region __TEST_FAILS__ // regions for tests that will & must fail

#pragma endregion __TEST_FAILS__

    return EXIT_SUCCESS;
}

// #endif // __TEST_BMPT_ASCII__
