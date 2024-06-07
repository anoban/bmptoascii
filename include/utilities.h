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

#include <math.h>
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
// we cannot make templates work with both RGBTRIPLE & RGBQUAD because they have different member names!

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

// ASCII characters in ascending order of luminance
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

#ifdef __WANT_PRIMITIVE_TRANSFORMERS__

// arithmetic average of an RGB pixel values
static inline unsigned __stdcall arithmetic_average(_In_ const RGBQUAD* const restrict _pixel) {
    // we don't want overflows or truncations here
    return (((double) (_pixel->rgbBlue)) + _pixel->rgbGreen + _pixel->rgbRed) / 3.000;
}

// weighted average of an RGB pixel values
static inline unsigned __stdcall weighted_average(_In_ const RGBQUAD* const restrict _pixel) {
    return _pixel->rgbBlue * 0.299 + _pixel->rgbGreen * 0.587 + _pixel->rgbRed * 0.114;
}

// average of minimum and maximum RGB values in a pixel
static inline unsigned __stdcall minmax_average(_In_ const RGBQUAD* const restrict _pixel) {
    // we don't want overflows or truncations here
    return (((double) (min(min(_pixel->rgbBlue, _pixel->rgbGreen), _pixel->rgbRed))) +
            (max(max(_pixel->rgbBlue, _pixel->rgbGreen), _pixel->rgbRed))) /
           2.0000;
}

// luminosity of an RGB pixel
static inline unsigned __stdcall luminosity(_In_ const RGBQUAD* const restrict _pixel) {
    return _pixel->rgbBlue * 0.2126 + _pixel->rgbGreen * 0.7152 + _pixel->rgbRed * 0.0722;
}

#endif // __WANT_PRIMITIVE_TRANSFORMERS__

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// transformers that map an RGB pixel to a representative unicode character, using the provided palette //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// one concern about these mappers is that the logic _result / (float) UCHAR_MAX could be any value between 0.0000 and 1.000
// because _result can be anywhere between 0 and 255!
// when it comes to mapping this pixel value to a character, multiplication by palette length can yield very small values, even for the longest palette
// the longest palette we have is the palette_extended, which is 70 characters long
// consider when _result is non-zero but still very small, e.g. _result = 1,
// ((_result / (float) (UCHAR_MAX)) evaluates to 1 / 255.0 = 0.00392156862745098
// that multiplied by palette length, 0.00392156862745098 * 70 = 0.274509803921569
// when we cast this to an unsigned, we get 0 due to truncation,
// subtract 1 from this, we end up with -1, a gurantted ticket to access violation!
// this logic return _palette[_result ? (unsigned) ((_result / (float) (UCHAR_MAX)) * _palette_len) - 1 : 0]; is dangerous!
// ceiling the subexpression ((_result / (float) (UCHAR_MAX)) * _palette_len) can help here!

// but ceil() will send a call to a ucrt dll every time we use the mapper in a loop, againt will make the performance phenomenally bad
// we really do not need a ceilf() call, all we need is a function that can return 1.000 when the input is between 0.00000 and 1.0000
// a handrolled inline function will be the best choice!

// taking it for granted that the input will never be a negative value,
static inline unsigned __stdcall nudge(_In_ const float _value) { return _value < 1.000000 ? 1 : _value; }

static inline wchar_t __stdcall arithmetic_mapper(
    _In_ const RGBQUAD* const restrict _pixel, _In_ const wchar_t* const restrict _palette, _In_ const unsigned _palette_len
) {
    const unsigned _result = (((double) (_pixel->rgbBlue)) + _pixel->rgbGreen + _pixel->rgbRed) / 3.000; // can range from 0 to 255
    // hence, _result / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall weighted_mapper(
    _In_ const RGBQUAD* const restrict _pixel, _In_ const wchar_t* const restrict _palette, _In_ const unsigned _palette_len
) {
    const unsigned _result = _pixel->rgbBlue * 0.299 + _pixel->rgbGreen * 0.587 + _pixel->rgbRed * 0.114;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall minmax_mapper(
    _In_ const RGBQUAD* const restrict _pixel, _In_ const wchar_t* const restrict _palette, _In_ const unsigned _palette_len
) {
    const unsigned _result = (((double) (min(min(_pixel->rgbBlue, _pixel->rgbGreen), _pixel->rgbRed))) +
                              (max(max(_pixel->rgbBlue, _pixel->rgbGreen), _pixel->rgbRed))) /
                             2.0000;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall luminosity_mapper(
    _In_ const RGBQUAD* const restrict _pixel, _In_ const wchar_t* const restrict _palette, _In_ const unsigned _palette_len
) {
    const unsigned _result = _pixel->rgbBlue * 0.2126 + _pixel->rgbGreen * 0.7152 + _pixel->rgbRed * 0.0722;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

///////////////////////////////////////////////////////////////////////////////////////////
// specialized transformer adaptors, to be used with downscalled block based RGB mapping //
///////////////////////////////////////////////////////////////////////////////////////////

static inline wchar_t __stdcall arithmetic_blockmapper(
    _In_ const register double rgbBlue,
    _In_ const register double rgbGreen,
    _In_ const register double rgbRed,
    _In_ const wchar_t* const restrict _palette,
    _In_ const unsigned _palette_len
) {
    const unsigned _result = (rgbBlue + rgbGreen + rgbRed) / 3.000; // can range from 0 to 255
    // hence, _result / (float)(UCHAR_MAX) can range from 0.0 to 1.0
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall weighted_blockmapper(
    _In_ const register double rgbBlue,
    _In_ const register double rgbGreen,
    _In_ const register double rgbRed,
    _In_ const wchar_t* const restrict _palette,
    _In_ const unsigned _palette_len
) {
    const unsigned _result = rgbBlue * 0.299 + rgbGreen * 0.587 + rgbRed * 0.114;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall minmax_blockmapper(
    _In_ const register double rgbBlue,
    _In_ const register double rgbGreen,
    _In_ const register double rgbRed,
    _In_ const wchar_t* const restrict _palette,
    _In_ const unsigned _palette_len
) {
    const unsigned _result = (min(min(rgbBlue, rgbGreen), rgbRed) + max(max(rgbBlue, rgbGreen), rgbRed)) / 2.0000;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}

static inline wchar_t __stdcall luminosity_blockmapper(
    _In_ const register double rgbBlue,
    _In_ const register double rgbGreen,
    _In_ const register double rgbRed,
    _In_ const wchar_t* const restrict _palette,
    _In_ const unsigned _palette_len
) {
    const unsigned _result = rgbBlue * 0.2126 + rgbGreen * 0.7152 + rgbRed * 0.0722;
    return _palette[_result ? nudge(_result / (float) (UCHAR_MAX) *_palette_len) - 1 : 0];
}
