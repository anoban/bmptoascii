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

static inline uint8_t* open(_In_ const wchar_t* const restrict filename, _Inout_ unsigned* const restrict rbytes) {
    DWORD          nbytes      = 0;
    LARGE_INTEGER  liFsize     = { .QuadPart = 0LLU };
    const HANDLE64 hFile       = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    uint8_t*       buffer      = NULL;
    BOOL           bReadStatus = FALSE;

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
        fputws(L"Memory allocation error in utilitiesopen\n", stderr);
        goto GET_FILESIZE_ERR;
    }

    bReadStatus = ReadFile(hFile, buffer, liFsize.QuadPart, &nbytes, NULL);
    if (!bReadStatus) {
        fwprintf_s(stderr, L"Error %lu in ReadFile\n", GetLastError());
        goto GET_FILESIZE_ERR;
    }

    CloseHandle(hFile);
    *rbytes = nbytes;
    return buffer;

GET_FILESIZE_ERR:
    free(buffer);
    CloseHandle(hFile);
INVALID_HANDLE_ERR:
    *rbytes = 0;
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

// arithmetic average of an RGB pixel values
static inline unsigned __stdcall arithmetic_average(_In_ const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((double) (pixel->rgbBlue)) + pixel->rgbGreen + pixel->rgbRed) / 3.000;
}

// weighted average of an RGB pixel values
static inline unsigned __stdcall weighted_average(_In_ const RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.299 + pixel->rgbGreen * 0.587 + pixel->rgbRed * 0.114;
}

// average of minimum and maximum RGB values in a pixel
static inline unsigned minmax_average(_In_ const RGBQUAD* const restrict pixel) {
    // we don't want overflows or truncations here
    return (((double) (min(min(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) +
            (max(max(pixel->rgbBlue, pixel->rgbGreen), pixel->rgbRed))) /
           2.0000;
}

// luminosity of an RGB pixel
static inline unsigned luminosity(_In_ const RGBQUAD* const restrict pixel) {
    return pixel->rgbBlue * 0.2126 + pixel->rgbGreen * 0.7152 + pixel->rgbRed * 0.0722;
}

// a composer that uses a specified pair of palette and RGB to BW mapper to return an appropriate wchar_t
// gives the wchar_t corresponding to the provided RGB pixel
static inline wchar_t __stdcall mapper(const RGBQUAD* const restrict pixel) {
    // _rgbtransformer(pixel) can range from 0 to 255
    const auto _offset { _rgbtransformer(pixel) };
    // hence, _offset / static_cast<float>(stdnumeric_limits<unsigned char>max()) can range from 0.0 to 1.0
    return _palette[_offset ? (_offset / static_cast<float>(stdnumeric_limits<unsigned char> max()) * _palette_len) - 1 : 0];
}
