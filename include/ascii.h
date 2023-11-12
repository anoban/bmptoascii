#pragma once
#include <bmp.h>

// ASCII characters in descending order of luminance
// static const char ascii[] = { 'N', '@', '#', 'W', '$', '9', '8', '7', '6', '5', '4', '3', '2', '1',
//                               '?', '!', 'a', 'b', 'c', ';', ':', '+', '=', '-', ',', '.', '_' };

static const char ascii[] = { '_', '.', ',', '-', '=', '+', ':', ';', 'c', 'b', 'a', '!', '?', '1',
                              '2', '3', '4', '5', '6', '7', '8', '9', '$', 'W', '#', '@', 'N' };

static __forceinline char __stdcall ScaleRgbQuad(_In_ const RGBQUAD* const restrict pixel) {
    return ascii[((((long) pixel->rgbBlue) + pixel->rgbGreen + pixel->rgbRed) / 3) % 27];
}

typedef struct _ascii_t {
        const char*  buffer;
        const size_t length; // count of wchar_t s in the buffer.
} ascii_t;

static inline ascii_t GenerateASCIIBuffer(_In_ const WinBMP* const restrict image) {
    // TODO
    // we need to downscale the ascii art if the image is larger than the console window
    // a fullscreen cmd window is 215 chars wide and 50 chars high
    char* txtbuff = malloc((image->infhead.biHeight * image->infhead.biWidth + image->infhead.biHeight) * sizeof(char));
    if (!txtbuff) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return (ascii_t) { NULL, 0 };
    }

    size_t h = 0, w = 0;
    for (; h < image->infhead.biHeight; ++h) {
        w = 0;
        for (; w < image->infhead.biWidth; ++w)
            txtbuff[h * image->infhead.biWidth + w] = ScaleRgbQuad(&image->pixel_buffer[h * image->infhead.biWidth + w]);
        // txtbuff[h * w + 1] = '\n';
    }
    return (ascii_t) { txtbuff, (image->infhead.biHeight * image->infhead.biWidth + image->infhead.biHeight) };
}