#pragma once

#include <bmp.hpp>

// ASCII characters in ascending order of luminance
static constexpr wchar_t wascii[]     = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                          L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };

static constexpr wchar_t wascii_ext[] = { L' ',  L'.', L'\'', L'`', L'^', L'"', L',', L':', L';', L'I', L'l', L'!', L'i', L'>',
                                          L'<',  L'~', L'+',  L'_', L'-', L'?', L']', L'[', L'}', L'{', L'1', L')', L'(', L'|',
                                          L'\\', L'/', L't',  L'f', L'j', L'r', L'x', L'n', L'u', L'v', L'c', L'z', L'X', L'Y',
                                          L'U',  L'J', L'C',  L'L', L'Q', L'0', L'O', L'Z', L'm', L'w', L'q', L'p', L'd', L'b',
                                          L'k',  L'h', L'a',  L'o', L'*', L'#', L'M', L'W', L'&', L'8', L'%', L'B', L'@', L'$' };

static constexpr wchar_t wascii_[]    = { L' ', L'.', L'-', L',', L':', L'+', L'~', L';', L'(', L'%', L'x', L'1', L'*', L'n', L'u',
                                          L'T', L'3', L'J', L'5', L'$', L'S', L'4', L'F', L'P', L'G', L'O', L'V', L'X', L'E', L'Z',
                                          L'8', L'A', L'U', L'D', L'H', L'K', L'W', L'@', L'B', L'Q', L'#', L'0', L'M', L'N' };

#define char_array wascii_ext

// weighted averaging: (pix.BLUE * 0.299L) + (pix.GREEN * 0.587) + (pix.RED * 0.114)
[[nodiscard]] static constexpr wchar_t __stdcall __ScaleRgbQuadWAVG(_In_ const RGBQUAD* const pixel) {
    return char_array[(size_t) (pixel->rgbBlue * 0.299L + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L) % __crt_countof(char_array)];
}

// regular arithmetic average using integer division
[[nodiscard]] static constexpr wchar_t __stdcall __ScaleRgbQuadAVG(_In_ const RGBQUAD* const pixel) {
    return char_array[(((size_t) (pixel->rgbBlue) + pixel->rgbGreen + pixel->rgbRed) / 3) % __crt_countof(char_array)];
}

typedef struct buffer {
        const wchar_t* buffer;
        const size_t   length;                                           // count of wchar_t s in the buffer.
} buffer_t;

[[nodiscard]] static constexpr buffer_t GenerateASCIIRawBuffer(_In_ const WinBMP* const image) {
    const size_t   npixels = (size_t) image->infhead.biHeight * image->infhead.biWidth;
    const size_t   nwchars = npixels + (2LLU * image->infhead.biHeight); // one additional L'\r', L'\n' at the end of each line

    wchar_t* const txtbuff = malloc(nwchars * sizeof(wchar_t));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (buffer_t) { NULL, 0 };
    }

    // clang-format off
    /*
    The pixel data is organized in rows from bottom to top and, within each row, from left to right. Each row is called a "scan line".
    If the image height is given as a negative number, then the rows are ordered from top to bottom.
    In most contemporary .BMP images, the pixel ordering seems to be bottom up. i.e, 

    (pixel at the top left corner of the image) 
    this will be the last pixel --> 10 11 12 13 14 15 16 17 18 19 
                                    .............................
                                    00 01 02 03 04 05 06 07 08 09 <-- this is the first pixel in the buffer 
                                                                    (pixel at the bottom right corner of the image)
    */
    // clang-format on

    size_t caret = 0;
    // if pixels are ordered top down. i.e the first pixel in the buffer is the one at the top left corner of the image.
    if (image->infhead.biHeight < 0) {
        for (int64_t nrows = 0; nrows < image->infhead.biHeight; ++nrows) {
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ++ncols)
                txtbuff[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }
    }

    // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
    else {
        // traverse up along the height
        for (int64_t nrows = image->infhead.biHeight - 1LL; nrows >= 0; --nrows) {
            // traverse left to right inside "scan lines"
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ++ncols)
                txtbuff[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }
    }

    assert(caret == nwchars);
    return (buffer_t) { txtbuff, caret };
}

// Generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140
// chars) The total downscaling is completely predicated only on the image width, and the proportionate scaling effects will
// automatically apply to the image height.

static inline buffer_t GenerateASCIIDownScaledBuffer(_In_ const WinBMP* const image) {
    // downscaling needs to be done in pixel blocks.
    // each block will be represented by a single wchar_t
    const size_t   block_s   = ceill(image->infhead.biWidth / 140.0L);
    const size_t   block_dim = powl(block_s, 2.0000L);

    // We'd have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
    // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs

    const size_t   nwchars   = 142 /* 140 wchar_ts + CRLF */ * ceill(image->infhead.biHeight / (long double) block_s);
    wchar_t* const txtbuff   = malloc(nwchars * sizeof(wchar_t));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (buffer_t) { NULL, 0 };
    }

    long double avg_B = 0.0, avg_G = 0.0, avg_R = 0.0;
    size_t      caret = 0, offset = 0;

    if (image->infhead.biHeight < 0) {
        for (int64_t nrows = 0; nrows < image->infhead.biHeight; nrows += block_s) {    // start traversal at the bottom most scan line
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols += block_s) { // traverse left to right in scan lines

                // deal with blocks
                for (int64_t bh = nrows; bh < (nrows + block_s); ++bh) {
                    for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                        offset  = (bh * image->infhead.biWidth) + bw;
                        avg_B  += image->pixel_buffer[offset].rgbBlue;
                        avg_G  += image->pixel_buffer[offset].rgbGreen;
                        avg_R  += image->pixel_buffer[offset].rgbRed;
                    }
                }

                avg_B            /= block_dim;
                avg_G            /= block_dim;
                avg_R            /= block_dim;
                // pixel->rgbBlue * 0.299L + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L
                txtbuff[caret++]  = char_array[(size_t) (avg_B * 0.299L + avg_G * 0.587L + avg_R * 0.114L) % __crt_countof(char_array)];
                avg_B = avg_G = avg_R = 0.0L;
            }

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }
    } else {
        for (int64_t nrows = image->infhead.biHeight - 1LLU; nrows >= 0; nrows -= block_s) { // start traversal at the bottom most scan line
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols += block_s) {      // traverse left to right in scan lines

                // deal with blocks
                for (int64_t bh = nrows; bh > (nrows - block_s); --bh) {
                    for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                        offset  = (bh * image->infhead.biWidth) + bw;
                        avg_B  += image->pixel_buffer[offset].rgbBlue;
                        avg_G  += image->pixel_buffer[offset].rgbGreen;
                        avg_R  += image->pixel_buffer[offset].rgbRed;
                    }
                }

                avg_B            /= block_dim;
                avg_G            /= block_dim;
                avg_R            /= block_dim;
                txtbuff[caret++]  = char_array[(size_t) ((avg_B + avg_G + avg_R) / 3) % __crt_countof(char_array)];
                avg_B = avg_G = avg_R = 0.0L;
            }

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }
    }
    // wprintf_s(L"caret %5zu, nwchars %5zu\n", caret, nwchars);
    // assert(caret == nwchars); not likely :(
    return (buffer_t) { txtbuff, caret };
}

// a context dependent dispatcher for GenerateRawASCIIBuffer and GenerateDownScaledASCIIBuffer
static __forceinline buffer_t __stdcall GenerateASCIIBuffer(_In_ const WinBMP* const image) {
    return (image->infhead.biWidth <= 140) ? GenerateASCIIRawBuffer(image) : GenerateASCIIDownScaledBuffer(image);
}
