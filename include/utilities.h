#pragma once

// clang-format off
#define _AMD64_
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_MEAN
#include <windef.h>
#include <wingdi.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
// clang-format on

#define ONE 1.000000000F

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// this is how wingdi defines these two structs!!
// typedef struct tagRGBTRIPLE {
//         BYTE    rgbtBlue;
//         BYTE    rgbtGreen;
//         BYTE    rgbtRed;
// } RGBTRIPLE, *PRGBTRIPLE, NEAR *NPRGBTRIPLE, FAR *LPRGBTRIPLE;

// typedef struct tagRGBQUAD {
//         BYTE rgbBlue;
//         BYTE rgbGreen;
//         BYTE rgbRed;
//         BYTE rgbReserved;
// } RGBQUAD;

static inline uint8_t* __cdecl open(_In_ const wchar_t* const restrict filename, _Inout_ unsigned* const restrict rbytes) {
    LARGE_INTEGER liFsize = { .QuadPart = 0LLU };
    uint8_t*      buffer  = NULL;

    const HANDLE64 hFile  = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fwprintf_s(stderr, L"Error %lu in CreateFileW\n", GetLastError());
        goto INVALID_HANDLE_ERR;
    }

    if (!GetFileSizeEx(hFile, &liFsize)) {
        fwprintf_s(stderr, L"Error %lu in GetFileSizeEx\n", GetLastError());
        goto GET_FILESIZE_ERR;
    }

    buffer = malloc(liFsize.QuadPart);
    if (!buffer) {
        fputws(L"Memory allocation error in open\n", stderr);
        goto GET_FILESIZE_ERR;
    }

    const BOOL bReadStatus = ReadFile(hFile, buffer, liFsize.QuadPart, rbytes, NULL);
    if (!bReadStatus) {
        fwprintf_s(stderr, L"Error %lu in ReadFile\n", GetLastError());
        goto GET_FILESIZE_ERR;
    }

    CloseHandle(hFile);
    return buffer;

GET_FILESIZE_ERR:
    CloseHandle(hFile);
    free(buffer);
INVALID_HANDLE_ERR:
    return NULL;
}

#pragma region __PALETTES__
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
#pragma endregion

#ifdef __WANT_PRIMITIVE_TRANSFORMERS__
    #pragma region __PRIMITIVE_TRANSFORMERS__

// arithmetic average of an RGB pixel values
static inline unsigned __stdcall arithmetic_average(_In_ const register RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000;
}

// weighted average of an RGB pixel values
static inline unsigned __stdcall weighted_average(_In_ const register RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114;
}

// average of minimum and maximum RGB values in a pixel
static inline unsigned __stdcall minmax_average(_In_ const register RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
            (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
           2.0000;
}

// luminosity of an RGB pixel
static inline unsigned __stdcall luminosity(_In_ const register RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
}
    #pragma endregion
#endif // __WANT_PRIMITIVE_TRANSFORMERS__

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
static inline unsigned __stdcall nudge(_In_ const register float _value) { return _value < 1.000000 ? 1 : _value; }

#pragma region __BASIC_MAPPERS__

static inline wchar_t __stdcall arithmetic_mapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register wchar_t* const restrict palette,
    _In_ const register unsigned plength
) {
    const unsigned offset = (((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000; // can range from 0 to 255
    // hence, offset / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall weighted_mapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register wchar_t* const restrict palette,
    _In_ const register unsigned plength
) {
    const unsigned offset = pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall minmax_mapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register wchar_t* const restrict palette,
    _In_ const register unsigned plength
) {
    const unsigned offset = (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
                             (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
                            2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall luminosity_mapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register wchar_t* const restrict palette,
    _In_ const register unsigned plength
) {
    const unsigned offset = pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

// a custom mapper that allows for fine tuning the mapping logic
// to reduce the influence of blue pixel colour in the ascii mapping, one could tone it down by specifying a < 1.0 scale factor for blue pixels,
// this would reduce the influence blue pixels have on the mapped character
// PREREQUISITES :: all the scale factors must be within the range of 0 and 1 and the sum of all three of them should never go above 1.000
// POTENTIAL HAZARD :: access violations may happen when the above requisites are not met!
static inline wchar_t __stdcall tunable_mapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register float bscale, // scaling factor for blue
    _In_ const register float gscale, // scaling factor for green
    _In_ const register float rscale, // scaling factor for red
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    assert(bscale >= 0.000);
    assert(gscale >= 0.000);
    assert(rscale >= 0.000);
    assert((bscale + gscale + rscale) <= ONE);

    const unsigned offset = pixel->rgbBlue * bscale + pixel->rgbGreen * gscale + pixel->rgbRed * rscale;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

#pragma endregion

#pragma region __PENALIZING_MAPPERS__
// CAUTION :: THE PENALIZING GROUP OF MAPPERS CAN AFFECT THE PERFORMANCE SUBSTANTIALLY
// will penalize the offset by the penalty term when the pixel satisfies the criteria
// PREREQUISITES :: penalty must be a float in between [0.0, 1.0] (an inclusive range)
// if you don't want a certain colour to be considered for penalization, specify both limits for that colour as UCHAR_MAX (or any identical values)
static __forceinline wchar_t __stdcall penalizing_arithmeticmapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register uint8_t bllim, // lower limit for blue pixels
    _In_ const register uint8_t bulim, // upper limit for blue pixels
    _In_ const register uint8_t gllim,
    _In_ const register uint8_t gulim,
    _In_ const register uint8_t rllim,
    _In_ const register uint8_t rulim,
    _In_ const wchar_t* const restrict palette,
    _In_ const register unsigned plength,
    _In_ const register float    penalty
) {
    // this mapper is incredibly expensive compared to the alternatives and hence using this with a 0.000 penalty will be ridiculous!
    // because you'll be paying dearly for something you do not need!
    assert(penalty >= 0.00000 && penalty <= ONE);

    const bool penalize = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = ((((float) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000
                            ) /* an enclosing parens here because we do not want zero division exceptions */
                            * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __forceinline wchar_t __stdcall penalizing_weightedmapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register uint8_t bllim,
    _In_ const register uint8_t bulim,
    _In_ const register uint8_t gllim,
    _In_ const register uint8_t gulim,
    _In_ const register uint8_t rllim,
    _In_ const register uint8_t rulim,
    _In_ const wchar_t* const restrict palette,
    _In_ const register unsigned plength,
    _In_ const register float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = (pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __forceinline wchar_t __stdcall penalizing_minmaxmapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register uint8_t bllim,
    _In_ const register uint8_t bulim,
    _In_ const register uint8_t gllim,
    _In_ const register uint8_t gulim,
    _In_ const register uint8_t rllim,
    _In_ const register uint8_t rulim,
    _In_ const wchar_t* const restrict palette,
    _In_ const register unsigned plength,
    _In_ const register float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset = (((float) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
                             (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed)) / 2.0000) *
                            (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static __forceinline wchar_t __stdcall penalizing_luminositymapper(
    _In_ const register RGBQUAD* const restrict pixel,
    _In_ const register uint8_t bllim,
    _In_ const register uint8_t bulim,
    _In_ const register uint8_t gllim,
    _In_ const register uint8_t gulim,
    _In_ const register uint8_t rllim,
    _In_ const register uint8_t rulim,
    _In_ const wchar_t* const restrict palette,
    _In_ const register unsigned plength,
    _In_ const register float    penalty
) {
    assert(penalty >= 0.00000 && penalty <= ONE);
    const bool penalize = ((bllim != bulim) && (pixel->rgbBlue >= bllim) && (pixel->rgbBlue <= bulim)) ||
                          ((gllim != gulim) && (pixel->rgbGreen >= gllim) && (pixel->rgbGreen <= gulim)) ||
                          ((rllim != rulim) && (pixel->rgbRed >= rllim) && (pixel->rgbRed <= rulim));

    const unsigned offset =
        (pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722) * (penalize ? (ONE - penalty) : 1);
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

#pragma endregion

#pragma region __BLOCK_MAPPERS__

static inline wchar_t __stdcall arithmetic_blockmapper(
    _In_ const register float rgbBlue,
    _In_ const register float rgbGreen,
    _In_ const register float rgbRed,
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    const unsigned offset = (rgbBlue + rgbGreen + rgbRed) / 3.000; // can range from 0 to 255
    // hence, offset / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall weighted_blockmapper(
    _In_ const register float rgbBlue,
    _In_ const register float rgbGreen,
    _In_ const register float rgbRed,
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    const unsigned offset = rgbBlue * 0.299 + rgbGreen * 0.587 + rgbRed * 0.114;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall minmax_blockmapper(
    _In_ const register float rgbBlue,
    _In_ const register float rgbGreen,
    _In_ const register float rgbRed,
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    const unsigned offset = (min(min(rgbBlue, rgbGreen), rgbRed) + max(max(rgbBlue, rgbGreen), rgbRed)) / 2.0000;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall luminosity_blockmapper(
    _In_ const register float rgbBlue,
    _In_ const register float rgbGreen,
    _In_ const register float rgbRed,
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    const unsigned offset = rgbBlue * 0.2126 + rgbGreen * 0.7152 + rgbRed * 0.0722;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall tunable_blockmapper(
    _In_ const register float rgbBlue,
    _In_ const register float bscale,
    _In_ const register float rgbGreen,
    _In_ const register float gscale,
    _In_ const register float rgbRed,
    _In_ const register float rscale,
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned plength
) {
    assert(bscale >= 0.000);
    assert(gscale >= 0.000);
    assert(rscale >= 0.000);
    if ((bscale + gscale + rscale) > ONE) wprintf_s(L"%.10lf\n", bscale + gscale + rscale);
    assert((bscale + gscale + rscale) <= ONE);

    const unsigned offset = rgbBlue * bscale + rgbGreen * gscale + rgbRed * rscale;
    return palette[offset ? nudge(offset / (float) (UCHAR_MAX) *plength) - 1 : 0];
}

static inline wchar_t __stdcall penalizing_blockmapper(
    _In_ const register float   rgbBlue,
    _In_ const register float   rgbGreen,
    _In_ const register float   rgbRed,
    _In_ const register uint8_t bthresh, // penalty threshold for blue
    _In_ const register uint8_t gthresh, // penalty threshold for green
    _In_ const register uint8_t rthresh, // penalty threshold for red
    _In_ const wchar_t* const restrict palette,
    _In_ const unsigned       plength,
    _In_ const register float penalty
) { }

#pragma endregion