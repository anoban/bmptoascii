#pragma once
#include <bmp.h>

// ASCII characters in descending order of luminance
static const wchar_t wasciirev[] = {L'N',L'@',L'#',L'W',L'$',L'9',L'8',L'7',L'6',L'5',L'4',L'3',L'2',L'1',
                              L'?',L'!',L'a',L'b',L'c',L';',L':',L'+',L'=',L'-',L',',L'.',L'_' };

static const wchar_t wascii[]    = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                             L'2',L'3',L'4',L'5',L'6',L'7',L'8',L'9',L'$',L'W',L'#',L'@',L'N' };

static __forceinline wchar_t __stdcall ScaleRgbQuad(_In_ const RGBQUAD* const restrict pixel) {
    return wascii[((((long) pixel->rgbBlue) + pixel->rgbGreen + pixel->rgbRed) / 3) % 27];
}

typedef struct _ascii_t {
        const wchar_t* buffer;
        const size_t length; // count of wchar_t s in the buffer.
} ascii_t;

static inline ascii_t GenerateASCIIBuffer(_In_ const WinBMP* const restrict image) {
    // TODO
    // we need to downscale the ascii art if the image is larger than the console window
    // a fullscreen cmd window is 215 chars wide and 50 chars high
    wchar_t* txtbuff = malloc(((size_t) (image->infhead.biHeight) * image->infhead.biWidth + image->infhead.biHeight) * sizeof(wchar_t));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (ascii_t) { NULL, 0 };
    }

    // if pixels are ordered top down. i.e the first pixel in the buffer is the one at the top left corner of the image.
    if (image->infhead.biHeight < 0) {
        size_t h = 0, w = 0, offset = 0;
        for (; h < image->infhead.biHeight; ++h) {
            w = 0;
            for (; w < image->infhead.biWidth; ++w) {
                txtbuff[offset] = ScaleRgbQuad(&image->pixel_buffer[(h * (image->infhead.biWidth)) + w]);
                offset++;
            }
            txtbuff[++offset] = L'\n';
        }
    }

    // if pixels are ordered bottom up. i.e the first pixel in the buffer is the one at the bottom left corner of the image.
    // traversal needs to start from the last pixel.
    size_t caret = 0;
    if (image->infhead.biHeight >= 0) {
        int64_t h = image->infhead.biHeight, w = image->infhead.biWidth;
         //((size_t) image->infhead.biHeight) * image->infhead.biWidth + image->infhead.biHeight;
        for (; h >= 0 ; --h) {
            w = image->infhead.biWidth;
            for (; w >= 0; --w) {
                txtbuff[caret++] = ScaleRgbQuad(&image->pixel_buffer[(h * (image->infhead.biWidth)) - w]);
            }
            txtbuff[++caret] = L'\n';
        }
    }


    return (ascii_t) { txtbuff, caret };
}