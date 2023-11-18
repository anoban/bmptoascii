#pragma once
#include <bmp.h>

// ASCII characters in descending order of luminance
static const wchar_t wasciirev[] = { L'N', L'@', L'#', L'W', L'$', L'9', L'8', L'7', L'6', L'5', L'4', L'3', L'2', L'1',
                                     L'?', L'!', L'a', L'b', L'c', L';', L':', L'+', L'=', L'-', L',', L'.', L'_' };

static const wchar_t wascii[]    = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                     L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };

// Does a weighted averaging on RGB values: (pix.BLUE * 0.299L) + (pix.GREEN * 0.587) + (pix.RED * 0.114)
static __forceinline wchar_t __stdcall ScaleRgbQuad(_In_ const RGBQUAD* const restrict pixel) {
    return wascii[((unsigned short) ((pixel->rgbBlue * 0.299L) + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L)) % 27];
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
    In most contemporary .BMP images, the pixel ordering seems to be bottom up.
    i.e,
                                                                                (pixel at the top right corner of the image)
                                              10 11 12 13 14 15 16 17 18 19 <-- this will be the last pixel
    this is the first pixel in the buffer --> 00 01 02 03 04 05 06 07 08 09
    (pixel at the bottom left corner of the image)
    */

    // if pixels are ordered top down. i.e the first pixel in the buffer is the one at the top left corner of the image.
    if (image->infhead.biHeight < 0) { }

    // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
    size_t caret = 0;
    if (image->infhead.biHeight >= 0) {

        int64_t ncols = image->infhead.biWidth;
        for (int64_t nrows = image->infhead.biHeight; nrows > 0; --nrows) {
            ncols = image->infhead.biWidth;
            for (; ncols > 0; --ncols) txtbuff[caret++] = ScaleRgbQuad(&image->pixel_buffer[ncols * nrows]);
            txtbuff[caret] = L'\n';
            txtbuff[++caret] = L'\r';
            caret++;
        }

        assert(caret == nwchars);
    }

    return (buffer_t) { txtbuff, caret };
}