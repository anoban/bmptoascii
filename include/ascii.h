#pragma once
#ifndef __ASCII_H_
    #define __ASCII_H_
    #include <bmp.h>

// ASCII characters in descending order of luminance
static const wchar_t wascii[] = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                  L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };

// Does a weighted averaging on RGBQUAD values: (pix.BLUE * 0.299L) + (pix.GREEN * 0.587) + (pix.RED * 0.114)
static __forceinline wchar_t __stdcall ScaleRgbQuad(_In_ const RGBQUAD* const restrict pixel) {
    return wascii[(size_t) (pixel->rgbBlue * 0.299L + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L) % __crt_countof(wascii)];
}

typedef struct buffer {
        const wchar_t* buffer;
        const size_t   length; // count of wchar_t s in the buffer.
} buffer_t;

static inline buffer_t GenerateRawASCIIBuffer(_In_ const WinBMP* const restrict image) {
    if (image->infhead.biWidth > 140) {
        fwprintf_s(
            stderr,
            L"Error in %s @ line %d: BMP image too large!. Use GenerateDownScaledASCIIBuffer() instead, which implements image downscaling.\n",
            __FUNCTIONW__,
            __LINE__
        );
        return (buffer_t) { NULL, 0 };
    }

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
        for (int64_t nrows = 0; nrows < image->infhead.biHeight; nrows++) {
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols++) {
                ;
                ;
            }
        }
        return (buffer_t) { txtbuff, caret };
    }

    // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
    else {
        // traverse up along the height
        for (int64_t nrows = image->infhead.biHeight - 1LL; nrows >= 0; --nrows) {
            // traverse left to right inside "scan lines"
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols++)
                txtbuff[caret++] = ScaleRgbQuad(&image->pixel_buffer[(nrows * image->infhead.biWidth) + ncols]);

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }

        assert(caret == nwchars);
        return (buffer_t) { txtbuff, caret };
    }
}

// Generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140 chars)
// The total downscaling is completely predicated only on the image width, and the proportionate scaling effects will automatically apply to
// the image height.

static inline buffer_t GenerateDownScaledASCIIBuffer(_In_ const WinBMP* const restrict image) {
    // downscaling needs to be done in pixel blocks.
    const size_t block_w = (size_t) ceill(image->infhead.biWidth / 140.0L);
    const size_t block_h = (size_t) ceill(image->infhead.biHeight / 140.0L);

    // We'd have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent that block
    // as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) RGBQUADs

    /*
    e.g.

    If the image width is 1200 pixels, block_w will be ceil(1200 / 140) = ceil(8.57142857142857) = 9
    and if the height is 2300 pixels,  block_h will be ceil(2300 / 140) = ceil(16.4285714285714) = 17

    */

    const size_t nwchars = (image->infhead.biWidth / block_w) * (image->infhead.biHeight / block_h) + 2 /* that's for pixel blocks */ +
                           (image->infhead.biHeight / block_h) + 1 /* and that's for CRLFs */;
    wchar_t* const txtbuff = malloc(nwchars * sizeof(wchar_t));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (buffer_t) { NULL, 0 };
    }

    long double avg_B = 0.0L, avg_G = 0.0L, avg_R = 0.0L;
    size_t      caret = 0;

    for (int64_t nrows = image->infhead.biHeight - 1LLU; nrows >= 0; nrows -= block_h) { // start traversal at the bottom most scan line
        for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols += block_w) {      // traverse left to right in scan lines

            // deal with blocks
            for (int64_t bh = nrows; bh > (nrows - block_w); --bh) {
                for (int64_t bw = ncols; bw < (ncols + block_w); ++bw) {
                    avg_B += image->pixel_buffer[(bh * image->infhead.biWidth) + bw].rgbBlue;
                    avg_G += image->pixel_buffer[(bh * image->infhead.biWidth) + bw].rgbGreen;
                    avg_R += image->pixel_buffer[(bh * image->infhead.biWidth) + bw].rgbRed;
                }
            }

            txtbuff[caret++] = wascii[(size_t) (avg_B * 0.299L + avg_G * 0.587L + avg_R * 0.114L) % __crt_countof(wascii)];
            avg_B = avg_G = avg_R = 0.0L;
        }
        txtbuff[caret++] = L'\n';
        txtbuff[caret++] = L'\r';
    }

    // assert(caret == nwchars);
    return (buffer_t) { txtbuff, caret };
}

// a wrapper for GenerateRawASCIIBuffer and GenerateDownScaledASCIIBuffer
static __forceinline buffer_t __stdcall GenerateASCIIBuffer(_In_ const WinBMP* const restrict image) {
    if (image->infhead.biWidth < 140) return GenerateRawASCIIBuffer(image);
    return GenerateDownScaledASCIIBuffer(image);
}

#endif // !__ASCII_H_