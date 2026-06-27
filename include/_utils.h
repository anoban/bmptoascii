#pragma once

#define ONE 1.000000000F

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/fcntl.h>
#include <sys/stat.h>

#ifdef _DEBUG
    #define dbgwprintf_s(...) wprintf_s(__VA_ARGS__)
    #define DEBUG_EXEC(...)   (__VA_ARGS__)
#else
    #define dbgwprintf_s(...)
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
static const wchar_t palette_minimal[]  = { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c', L'b', L'a', L'!', L'?', L'1',
                                            L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };

static const wchar_t palette[]          = { L' ', L'.', L'-', L',', L':', L'+', L'~', L';', L'(', L'%', L'x', L'1', L'*', L'n', L'u',
                                            L'T', L'3', L'J', L'5', L'$', L'S', L'4', L'F', L'P', L'G', L'O', L'V', L'X', L'E', L'Z',
                                            L'8', L'A', L'U', L'D', L'H', L'K', L'W', L'@', L'B', L'Q', L'#', L'0', L'M', L'N' };

static const wchar_t palette_extended[] = { L' ',  L'.', L'\'', L'`', L'^', L'"', L',', L':', L';', L'I', L'l', L'!', L'i', L'>',
                                            L'<',  L'~', L'+',  L'_', L'-', L'?', L']', L'[', L'}', L'{', L'1', L')', L'(', L'|',
                                            L'\\', L'/', L't',  L'f', L'j', L'r', L'x', L'n', L'u', L'v', L'c', L'z', L'X', L'Y',
                                            L'U',  L'J', L'C',  L'L', L'Q', L'0', L'O', L'Z', L'm', L'w', L'q', L'p', L'd', L'b',
                                            L'k',  L'h', L'a',  L'o', L'*', L'#', L'M', L'W', L'&', L'8', L'%', L'B', L'@', L'$' };

// arithmetic average of an RGB pixel values
static inline unsigned arithmetic_average(const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000;
}

// weighted average of an RGB pixel values
static inline unsigned weighted_average(const RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114;
}

// average of minimum and maximum RGB values in a pixel
static inline unsigned minmax_average(const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
            (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
           2.0000;
}

// luminosity of an RGB pixel
static inline unsigned luminosity(const RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
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
static inline unsigned nudge(const float _value) { return _value < 1.000000 ? 1 : _value; }

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
                             (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
                            2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t luminosity_mapper(
    const RGBQUAD* const restrict pixel, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

// a custom mapper that allows for fine tuning the mapping logic
// to reduce the influence of blue pixel colour in the ascii mapping, one could tone it down by specifying a < 1.0 scale factor for blue pixels,
// this would reduce the influence blue pixels have on the mapped character
// PREREQUISITES  all the scale factors must be within the range of 0 and 1 and the sum of all three of them should never go above 1.000
// POTENTIAL HAZARD  access violations may happen when the above requisites are not met!
static inline wchar_t tunable_mapper(
    const RGBQUAD* const restrict pixel,
    const float bscale, // scaling factor for blue
    const float gscale, // scaling factor for green
    const float rscale, // scaling factor for red
    const wchar_t* const restrict palette,
    const unsigned plength
) {
    assert(bscale >= 0.000);
    assert(gscale >= 0.000);
    assert(rscale >= 0.000);
    assert((bscale + gscale + rscale) <= ONE);

    const unsigned offset = pixel->rgbBlue * bscale + pixel->rgbGreen * gscale + pixel->rgbRed * rscale;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

// CAUTION  THE PENALIZING GROUP OF MAPPERS CAN AFFECT THE PERFORMANCE SUBSTANTIALLY
// will penalize the offset by the penalty term when the pixel satisfies the criteria
// PREREQUISITES  penalty must be a float in between [0.0, 1.0] (an inclusive range)
// if you don't want a certain colour to be considered for penalization, specify both limits for that colour as UCHAR_MAX (or any identical values)
static __attribute__((always_inline)) wchar_t penalizing_arithmeticmapper(
    const RGBQUAD* const restrict pixel,
    const unsigned char bllim, // lower limit for blue pixels
    const unsigned char bulim, // upper limit for blue pixels
    const unsigned char gllim,
    const unsigned char gulim,
    const unsigned char rllim,
    const unsigned char rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    // this mapper is incredibly expensive compared to the alternatives and hence using this with a 0.000 penalty will be ridiculous!
    // because you'll be paying dearly for something you do not need!
    assert(penalty >= 0.00000 && penalty <= ONE);

    const bool penalize   = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                            ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                            ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = ((((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) /
                             3.000) /* an enclosing parens here because we do not want zero division exceptions */
                            * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_weightedmapper(
    const RGBQUAD* const restrict pixel,
    const unsigned char bllim,
    const unsigned char bulim,
    const unsigned char gllim,
    const unsigned char gulim,
    const unsigned char rllim,
    const unsigned char rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize   = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                            ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                            ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = (pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_minmaxmapper(
    const RGBQUAD* const restrict pixel,
    const unsigned char bllim,
    const unsigned char bulim,
    const unsigned char gllim,
    const unsigned char gulim,
    const unsigned char rllim,
    const unsigned char rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize   = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                            ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                            ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
                             (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed)) / 2.0000) *
                            (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_luminositymapper(
    const RGBQUAD* const restrict pixel,
    const unsigned char bllim,
    const unsigned char bulim,
    const unsigned char gllim,
    const unsigned char gulim,
    const unsigned char rllim,
    const unsigned char rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset =
        (pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722) * (penalize ? (ONE - penalty) : 1);
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
    const unsigned offset = (min(min(rgbBlue, rgbGreen), rgbRed) + max(max(rgbBlue, rgbGreen), rgbRed)) / 2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t luminosity_blockmapper(
    const float rgbBlue, const float rgbGreen, const float rgbRed, const wchar_t* const restrict palette, const unsigned plength
) {
    const unsigned offset = rgbBlue * 0.2126 + rgbGreen * 0.7152 + rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t tunable_blockmapper(
    const float rgbBlue,
    const float bscale,
    const float rgbGreen,
    const float gscale,
    const float rgbRed,
    const float rscale,
    const wchar_t* const restrict palette,
    const unsigned plength
) {
    assert(bscale >= 0.000);
    assert(gscale >= 0.000);
    assert(rscale >= 0.000);
    if ((bscale + gscale + rscale) > ONE) wprintf_s(L"%.10lf\n", bscale + gscale + rscale);
    assert((bscale + gscale + rscale) <= ONE);

    const unsigned offset = rgbBlue * bscale + rgbGreen * gscale + rgbRed * rscale;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_arithmeticblockmapper(
    const float rgbBlue,
    const float rgbGreen,
    const float rgbRed,
    const float bllim,
    const float bulim,
    const float gllim,
    const float gulim,
    const float rllim,
    const float rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    // this mapper is incredibly expensive compared to the alternatives and hence using this with a 0.000 penalty will be ridiculous!
    // because you'll be paying dearly for something you do not need!
    assert(penalty >= 0.00000 && penalty <= ONE);

    const bool penalize = ((bllim != bulim) && (rgbBlue >= bllim) && (rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (rgbGreen >= gllim) && (rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (rgbRed >= rllim) && (rgbRed <= rulim));

    const unsigned offset =
        ((rgbBlue + rgbGreen + rgbRed) / 3.000) /* an enclosing parens here because we do not want zero division exceptions */
        * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_weightedblockmapper(
    const float rgbBlue,
    const float rgbGreen,
    const float rgbRed,
    const float bllim,
    const float bulim,
    const float gllim,
    const float gulim,
    const float rllim,
    const float rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize   = ((bllim != bulim) && (rgbBlue >= bllim) && (rgbBlue <= bulim)) ||
                            ((gllim != gulim) && (rgbGreen >= gllim) && (rgbGreen <= gulim)) ||
                            ((rllim != rulim) && (rgbRed >= rllim) && (rgbRed <= rulim));

    const unsigned offset = (rgbBlue * 0.299 + rgbGreen * 0.587 + rgbRed * 0.114) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_minmaxblockmapper(
    const float rgbBlue,
    const float rgbGreen,
    const float rgbRed,
    const float bllim,
    const float bulim,
    const float gllim,
    const float gulim,
    const float rllim,
    const float rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize = ((bllim != bulim) && (rgbBlue >= bllim) && (rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (rgbGreen >= gllim) && (rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (rgbRed >= rllim) && (rgbRed <= rulim));

    const unsigned offset =
        ((min(min(rgbBlue, rgbGreen), rgbRed) + max(max(rgbBlue, rgbGreen), rgbRed)) / 2.0000) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __attribute__((always_inline)) wchar_t penalizing_luminosityblockmapper(
    const float rgbBlue,
    const float rgbGreen,
    const float rgbRed,
    const float bllim,
    const float bulim,
    const float gllim,
    const float gulim,
    const float rllim,
    const float rulim,
    const wchar_t* const restrict palette,
    const unsigned plength,
    const float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize   = ((bllim != bulim) && (rgbBlue >= bllim) && (rgbBlue <= bulim)) ||
                            ((gllim != gulim) && (rgbGreen >= gllim) && (rgbGreen <= gulim)) ||
                            ((rllim != rulim) && (rgbRed >= rllim) && (rgbRed <= rulim));

    const unsigned offset = (rgbBlue * 0.2126 + rgbGreen * 0.7152 + rgbRed * 0.0722) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}
