// clang .\src\test.c -I .\include\ -Wall -Wextra -O3 -pedantic -march=native -std=c23 -D__TEST_BMPT_ASCII__

#ifdef __TEST_BMPT_ASCII__
    #define __WANT_PRIMITIVE_TRANSFORMERS__

    #include <tostring.h>

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

static const RGBQUAD min                    = { .rgbBlue = 0x00, .rgbGreen = 0x00, .rgbRed = 0x00, .rgbReserved = 0xFF };
static const RGBQUAD mid                    = { .rgbBlue = 0x80, .rgbGreen = 0x80, .rgbRed = 0x80, .rgbReserved = 0xFF };
static const RGBQUAD max                    = { .rgbBlue = 0xFF, .rgbGreen = 0xFF, .rgbRed = 0xFF, .rgbReserved = 0xFF };

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

int wmain(void) {
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

    RGBQUAD test = { 0x00, 0x00, 0x00, 0xFF };

    for (unsigned blue = 0; blue <= UCHAR_MAX; ++blue) {
        for (unsigned green = 0; green <= UCHAR_MAX; ++green) {
            for (unsigned red = 0; red <= UCHAR_MAX; ++red) {
                test.rgbBlue  = blue;
                test.rgbGreen = green;
                test.rgbRed   = red;
                // unsigned returns will always be greater than or equal to 0, so not testing against 0
                assert(arithmetic_average(&test) <= UCHAR_MAX);
                assert(weighted_average(&test) <= UCHAR_MAX);
                assert(minmax_average(&test) <= UCHAR_MAX);
                assert(luminosity(&test) <= UCHAR_MAX);
            }
        }
    }
    #pragma endregion __TEST_TRANSFORMERS__

    #pragma region __TEST_RGBMAPPER__
    RGBQUAD        temp = { 0 };

    for (unsigned blue = 0; blue <= UCHAR_MAX; ++blue) {
        for (unsigned green = 0; green <= UCHAR_MAX; ++green) {
            for (unsigned red = 0; red <= UCHAR_MAX; ++red) {
                temp.rgbBlue  = blue;
                temp.rgbGreen = green;
                temp.rgbRed   = red;

                // make sure none of the below raise an access violation exception!
                arithmetic_mapper(&temp, palette, __crt_countof(palette));
                arithmetic_mapper(&temp, palette_minimal, __crt_countof(palette_minimal));
                arithmetic_mapper(&temp, palette_extended, __crt_countof(palette_extended));

                weighted_mapper(&temp, palette, __crt_countof(palette));
                weighted_mapper(&temp, palette_minimal, __crt_countof(palette_minimal));
                weighted_mapper(&temp, palette_extended, __crt_countof(palette_extended));

                minmax_mapper(&temp, palette, __crt_countof(palette));
                minmax_mapper(&temp, palette_minimal, __crt_countof(palette_minimal));
                minmax_mapper(&temp, palette_extended, __crt_countof(palette_extended));

                luminosity_mapper(&temp, palette, __crt_countof(palette));
                luminosity_mapper(&temp, palette_minimal, __crt_countof(palette_minimal));
                luminosity_mapper(&temp, palette_extended, __crt_countof(palette_extended));
            }
        }
    }

    for (unsigned blue = 0; blue <= UCHAR_MAX; ++blue) {
        for (unsigned green = 0; green <= UCHAR_MAX; ++green) {
            for (unsigned red = 0; red <= UCHAR_MAX; ++red) {
                // make sure none of the below raise an access violation exception!
                arithmetic_blockmapper(blue, green, red, palette, __crt_countof(palette));
                arithmetic_blockmapper(blue, green, red, palette_minimal, __crt_countof(palette_minimal));
                arithmetic_blockmapper(blue, green, red, palette_extended, __crt_countof(palette_extended));

                weighted_blockmapper(blue, green, red, palette, __crt_countof(palette));
                weighted_blockmapper(blue, green, red, palette_minimal, __crt_countof(palette_minimal));
                weighted_blockmapper(blue, green, red, palette_extended, __crt_countof(palette_extended));

                minmax_blockmapper(blue, green, red, palette, __crt_countof(palette));
                minmax_blockmapper(blue, green, red, palette_minimal, __crt_countof(palette_minimal));
                minmax_blockmapper(blue, green, red, palette_extended, __crt_countof(palette_extended));

                luminosity_blockmapper(blue, green, red, palette, __crt_countof(palette));
                luminosity_blockmapper(blue, green, red, palette_minimal, __crt_countof(palette_minimal));
                luminosity_blockmapper(blue, green, red, palette_extended, __crt_countof(palette_extended));
            }
        }
    }
    #pragma endregion __TEST_RGBMAPPER__

    #pragma region         __TEST_PARSERS__
    const BITMAPFILEHEADER bmpfh = parse_fileheader(dummybmp, __crt_countof(dummybmp));
    assert(bmpfh.bfType == start_tag_le);
    assert(bmpfh.bfSize == 1409334); // size of the image where this buffer was extracted from, in bytes
    assert(bmpfh.bfReserved1 == 0);
    assert(bmpfh.bfReserved2 == 0);
    assert(bmpfh.bfOffBits == 54);

    const BITMAPINFOHEADER bmpinfh = parse_infoheader(dummybmp, __crt_countof(dummybmp));
    assert(bmpinfh.biSize == 40); // header size
    assert(bmpinfh.biWidth == 734);
    assert(bmpinfh.biHeight == 480);
    assert(bmpinfh.biPlanes == 1);
    assert(bmpinfh.biBitCount == 32);
    assert(bmpinfh.biCompression == 0);
    assert(bmpinfh.biSizeImage == 0);
    assert(bmpinfh.biXPelsPerMeter == 3780);
    assert(bmpinfh.biYPelsPerMeter == 3780);
    assert(bmpinfh.biClrUsed == 0);
    assert(bmpinfh.biClrImportant == 0);

    const BITMAP_PIXEL_ORDERING order = get_pixel_order(&bmpinfh);
    assert(order == BOTTOMUP);
    #pragma endregion __TEST_PARSERS__

    #pragma region __TEST_FULL__
    bitmap_t       vendetta                    = bitmap_read(L"./test/vendetta.bmp");
    const wchar_t* const restrict vendettawstr = to_string(&vendetta);
    _putws(vendettawstr);
    _putws(L"\n\n");

    bitmap_t child                          = bitmap_read(L"./test/child.bmp");
    const wchar_t* const restrict childwstr = to_string(&child);
    _putws(childwstr);
    _putws(L"\n\n");

    bitmap_t girl                          = bitmap_read(L"./test/girl.bmp");
    const wchar_t* const restrict girlwstr = to_string(&girl);
    _putws(girlwstr);
    _putws(L"\n\n");

    bitmap_t bobmarley                          = bitmap_read(L"./test/bobmarley.bmp");
    const wchar_t* const restrict bobmarleywstr = to_string(&bobmarley);
    _putws(bobmarleywstr);
    _putws(L"\n\n");

    bitmap_t cubes                          = bitmap_read(L"./test/cubes.bmp");
    const wchar_t* const restrict cubeswstr = to_string(&cubes);
    _putws(cubeswstr);
    _putws(L"\n\n");

    free(vendettawstr);
    free(childwstr);
    free(girlwstr);
    free(bobmarleywstr);
    free(cubeswstr);

    bitmap_close(&vendetta);
    bitmap_close(&child);
    bitmap_close(&girl);
    bitmap_close(&bobmarley);
    bitmap_close(&cubes);
    #pragma endregion __TEST_FULL__

    _putws(L"all's good :)");
    return EXIT_SUCCESS;
}

#endif // __TEST_BMPT_ASCII__
