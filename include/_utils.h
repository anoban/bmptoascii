#pragma once

#define ONE       1.000000000
#define min(x, y) (((x) < (y) ? (x) : (y)))
#define max(x, y) (((x) > (y) ? (x) : (y)))

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/stat.h>

// clang-format off
#include <_wingdi.h>
// clang-format on

#ifdef _DEBUG
    #define __printf_debug(...) printf(__VA_ARGS__)
    #define DEBUG_EXEC(...)     (__VA_ARGS__)
#else
    #define __printf_debug(...)
    #define DEBUG_EXEC(...)
#endif // _DEBUG

static inline unsigned char* imopen(const char* const fpath, long* const nreadbytes) {
    *nreadbytes             = 0;
    unsigned char* buffer   = NULL;
    struct stat    filestat = {};
    long           nbytes   = 0;

    const int fdesc         = open(fpath, O_RDONLY);
    if (fdesc == -1) { // if open() failed, the return value will be -1
        fprintf(stderr, "Call to open() failed inside %s at line %d!; errno %d\n", __FUNCTION__, __LINE__, errno);
        return NULL;
    }

    if (fstat(fdesc, &filestat)) { // if succeeds, 0 is returned, -1 if fails
        fprintf(stderr, "Call to fstat() failed inside %s at line %d!; errno %d\n", __FUNCTION__, __LINE__, errno);
        goto CLOSE_AND_RETURN;
    }

    if (!(buffer = malloc(filestat.st_size))) { // caller is responsible for freeing this buffer
        fprintf(stderr, "Call to new() failed inside %s at line %d!\n", __FUNCTION__, __LINE__);
        goto CLOSE_AND_RETURN;
    }

    if ((nbytes = read(fdesc, buffer, filestat.st_size)) != -1) {
        *nreadbytes = nbytes;
        assert(nbytes == filestat.st_size); // double checking
    } else {
        fprintf(stderr, "Call to read() failed inside %s at line %d!; errno %d\n", __FUNCTION__, __LINE__, errno);
        free(buffer);
        buffer = NULL;
    }
    // then, fall through the CLOSE_AND_RETURN label

CLOSE_AND_RETURN:
    // close() returns 0 on success and -1 on failure
    if (close(fdesc)) fprintf(stderr, "Call to close() failed inside %s at line %d!; errno %d\n", __FUNCTION__, __LINE__, errno);
    return buffer;
}

// characters in ascending order of luminance
static const char palette_minimal[]  = { '_', '.', ',', '-', '=', '+', ':', ';', 'c', 'b', 'a', '!', '?', '1',
                                         '2', '3', '4', '5', '6', '7', '8', '9', '$', 'W', '#', '@', 'N' };

static const char palette_base[]     = { ' ', '.', '-', ',', ':', '+', '~', ';', '(', '%', 'x', '1', '*', 'n', 'u',
                                         'T', '3', 'J', '5', '$', 'S', '4', 'F', 'P', 'G', 'O', 'V', 'X', 'E', 'Z',
                                         '8', 'A', 'U', 'D', 'H', 'K', 'W', '@', 'B', 'Q', '#', '0', 'M', 'N' };

static const char palette_extended[] = { ' ', '.', '\'', '`', '^', '"', ',', ':', ';', 'I',  'l',  '!', 'i', '>', '<', '~', '+', '_',
                                         '-', '?', ']',  '[', '}', '{', '1', ')', '(', '|',  '\\', '/', 't', 'f', 'j', 'r', 'x', 'n',
                                         'u', 'v', 'c',  'z', 'X', 'Y', 'U', 'J', 'C', '\'', 'Q',  '0', 'O', 'Z', 'm', 'w', 'q', 'p',
                                         'd', 'b', 'k',  'h', 'a', 'o', '*', '#', 'M', 'W',  '&',  '8', '%', 'B', '@', '$' };

// arithmetic average of an RGB pixel values
static inline unsigned arithmetic(const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (unsigned) ((((double) pixel->rgbBlue) + pixel->rgbGreen + pixel->rgbRed) / 3.000);
}

// weighted average of an RGB pixel values
static inline unsigned weighted(const RGBQUAD* const restrict pixel) {
    return (unsigned) (pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114);
}

// average of minimum and maximum RGB values in a pixel
static inline unsigned minmax(const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (unsigned) ((((double) min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed)) +
                        max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed)) /
                       2.0000);
}

// luminosity of an RGB pixel
static inline unsigned luminosity(const RGBQUAD* const restrict pixel) {
    return (unsigned) (pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// transformers that map an RGB pixel to a representative unicode character, using the provided palette //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// one concern about these mappers is that the logic offset / (float) UCHAR_MAX could be any value between 0.0000 and 1.000
// because offset can be anywhere between 0 and 255!
// when it comes to mapping this pixel value to a character, multiplication by palette length can yield very small values, even for the longest palette
// the longest palette we have is the palette_extended, which is 70 characters long
// consider when offset is non-zero but still very small, e.g. offset = 1,
// ((offset / (float) (UCHAR_MAX)) evaluates to 1 / 255.0 = 0.00392156862745098
// that multiplied by palette length, 0.00392156862745098 * 70 = 0.274509803921569
// when we cast this to an unsigned, we get 0 due to truncation,
// subtract 1 from this, we end up with -1, a gurantted ticket to access violation!
// this logic return palette[offset ? (unsigned) ((offset / (float) (UCHAR_MAX)) * plength) - 1 : 0]; is dangerous!
// ceiling the subexpression ((offset / (float) (UCHAR_MAX)) * plength) can help here!

// but ceil() will send a call to a ucrt dll every time we use the mapper in a loop, againt will make the performance phenomenally bad
// we really do not need a ceilf() call, all we need is a function that can return 1.000 when the input is between 0.00000 and 1.0000
// a handrolled inline function will be the best choice!

// taking it for granted that the input will never be a negative value,
static inline unsigned nudge(const float _value) { return _value < 1.000000 ? 1 : (unsigned) _value; }

static inline wchar_t arithmetic_mapper(
    const RGBQUAD* const restrict pixel, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = (((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000; // can range from 0 to 255
    // hence, offset / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t weighted_mapper(const RGBQUAD* const restrict pixel, const wchar_t* const restrict palette, const unsigned plength) {
    const unsigned offset = pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t minmax_mapper(const RGBQUAD* const restrict pixel, const wchar_t* const restrict palette, const unsigned plength) {
    const unsigned offset = (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
                             (fmax(fmax(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
                            2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t luminosity_mapper(
    const RGBQUAD* const restrict pixel, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t arithmetic_blockmapper(
    const float rgbBlue, const float rgbGreen, const float rgbRed, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = (rgbBlue + rgbGreen + rgbRed) / 3.000; // can range from 0 to 255
    // hence, offset / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t weighted_blockmapper(
    const float rgbBlue, const float rgbGreen, const float rgbRed, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = rgbBlue * 0.299 + rgbGreen * 0.587 + rgbRed * 0.114;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t minmax_blockmapper(
    const float rgbBlue, const float rgbGreen, const float rgbRed, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = (min(min(rgbBlue, rgbGreen), rgbRed) + fmax(max(rgbBlue, rgbGreen), rgbRed)) / 2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t luminosity_blockmapper(
    const float rgbBlue, const float rgbGreen, const float rgbRed, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = rgbBlue * 0.2126 + rgbGreen * 0.7152 + rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}
