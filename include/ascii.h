#pragma once
#ifndef __ASCII_H_
    #define __ASCII_H_

    #include <bmp.h>

// ASCII characters in descending order of luminance
<<<<<<< HEAD
static const wchar_t wascii[] = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                  L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };
=======
static const wchar_t wascii[]     = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                      L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };
>>>>>>> 64d730bdc42cf14254a7ff67de91da9b1bceaa22

// Does a weighted averaging on RGB values: (pix.BLUE * 0.299L) + (pix.GREEN * 0.587) + (pix.RED * 0.114)
static __forceinline wchar_t __stdcall ScaleRgbQuad(_In_ const RGBQUAD* const restrict pixel) {
    return wascii
        [((unsigned short) ((pixel->rgbBlue * 0.299L) + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L)) % __crt_countof(wascii)];
}

typedef struct buffer {
        const wchar_t* buffer;
        const size_t   length; // count of wchar_t s in the buffer.
} buffer_t;

static inline buffer_t GenerateASCIIBuffer(_In_ const WinBMP* const restrict image) {
    // TODO
    // we need to downscale the ascii art if the image is larger than the console window
    // a fullscreen cmd window is 215 chars wide and 50 chars high

    const size_t npixels = (size_t) image->infhead.biHeight * image->infhead.biWidth;
    const size_t nwchars = npixels + (2LLU * image->infhead.biHeight); // one additional L'\r', L'\n' at the end of each line

    wchar_t*     txtbuff = malloc(nwchars * sizeof(wchar_t));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (buffer_t) { NULL, 0 };
    }

    /*
    The pixel data is organized in rows from bottom to top and, within each row, from left to right. Each row is called a "scan line".
    If the image height is given as a negative number, then the rows are ordered from top to bottom.
    In most contemporary .BMP images, the pixel ordering seems to be bottom up. i.e, (pixel at the top right corner of the image) this will
    be the last pixel --> 10 11 12 13 14 15 16 17 18 19 00 01 02 03 04 05 06 07 08 09 <-- this is the first pixel in the buffer (pixel at
    the bottom left corner of the image)
    */

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
        for (int64_t nrows = image->infhead.biHeight - 1; nrows >= 0; --nrows) {
            // traverse left to right inside "scan lines"
            for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols++)
                txtbuff[caret++] = ScaleRgbQuad(&image->pixel_buffer[ncols + (nrows * image->infhead.biWidth)]);

            txtbuff[caret++] = L'\n';
            txtbuff[caret++] = L'\r';
        }

        assert(caret == nwchars);
        return (buffer_t) { txtbuff, caret };
    }
}

// Generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140 chars)
// Image height is not catered to here!
static inline buffer_t GenerateDownScaledASCIIBuffer(_In_ const WinBMP* const restrict image) {
    // downscaling factor, one wchar_t in our buffer will have to represent this many RGBQUADs in a scan line.
    const size_t window_w               = ceill(image->infhead.biWidth / 140.0L);

    /*
    If the image width is 1200 pixels, window_w will be ceil(1200 / 140) = ceil(8.57142857142857) = 9
    So we'll combine the averages of 9 subsequent pixels to represent by a wchar_t.
    */

    const size_t nwchars /* per line */ = (image->infhead.biWidth / window_w) /* intentional truncation */ + 1;
    // = (1200 / 9) + 1
    // = trunc(133.333333333333) + 1
    // = 133 + 1
    // = 134
}

#endif //__ASCII_H_