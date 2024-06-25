#pragma once

#include <assert.h>
#include <stdbool.h>
#include <utilities.h>

// a struct representing a windows bitmap
typedef struct bitmap {
        BITMAPFILEHEADER _fileheader;
        BITMAPINFOHEADER _infoheader;
        RGBQUAD*         _pixels; // this points to the start of pixels in the file buffer i.e (_buffer + 54)
        // _pixels IS NOT A SEPARATE BUFFER, IT IS JUST A REFERENCE TO A BYTE FEW STRIDES (54 BYTES) INTO THE ACTUAL BYTES BUFFER
        uint8_t*         _buffer; // this will point to the original file buffer, this is the one that needs deallocation!
} bitmap_t;

// order of pixels in the BMP buffer.
typedef enum { TOPDOWN, BOTTOMUP } BITMAP_PIXEL_ORDERING;

// types of compressions used in BMP files.
typedef enum { RGB, RLE8, RLE4, BITFIELDS, UNKNOWN } BITMAP_COMPRESSION_KIND;

// BMP files store this tag as 'B', followed by 'M', i.e 0x424D as an unsigned 16 bit integer,
// when we dereference this 16 bits as an unsigned 16 bit integer on LE machines, the byte order will get swapped i.e the two bytes will be read as 'M', 'B'
static const unsigned short START_TAG_BE = L'B' << 8 | L'M';
static const unsigned short START_TAG_LE = L'M' << 8 | L'B';

static BITMAPFILEHEADER __cdecl parse_fileheader(_In_ const uint8_t* const restrict imstream, _In_ const unsigned size) {
    assert(size >= sizeof(BITMAPFILEHEADER));
    BITMAPFILEHEADER header = { .bfType = 0, .bfSize = 0, .bfReserved1 = 0, .bfReserved2 = 0, .bfOffBits = 0 };

    if (*((uint16_t*) (imstream)) != START_TAG_LE) {
        fputws(L"Error in parse_fileheader, file appears not to be a Windows BMP file\n", stderr);
        free(imstream);
        return header;
    }
    header.bfType    = START_TAG_LE;
    header.bfSize    = *(uint32_t*) (imstream + 2);
    header.bfOffBits = *(uint32_t*) (imstream + 10);
    return header;
}

static inline BITMAPINFOHEADER __cdecl parse_infoheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) {
    assert(size >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));
    BITMAPINFOHEADER header = { 0 };

    if (*((uint32_t*) (imstream + 14U)) > 40U) {
        fputws(L"Error in parse_infoheader, BMP image seems to contain an unparsable file info header", stderr);
        free(imstream);
        return header;
    }

    header.biSize          = *((uint32_t*) (imstream + 14U));
    header.biWidth         = *((uint32_t*) (imstream + 18U));
    header.biHeight        = *((int32_t*) (imstream + 22U));
    header.biPlanes        = *((uint16_t*) (imstream + 26U));
    header.biBitCount      = *((uint16_t*) (imstream + 28U));
    header.biCompression   = *((uint32_t*) (imstream + 30U));
    header.biSizeImage     = *((uint32_t*) (imstream + 34U));
    header.biXPelsPerMeter = *((uint32_t*) (imstream + 38U));
    header.biYPelsPerMeter = *((uint32_t*) (imstream + 42U));
    header.biClrUsed       = *((uint32_t*) (imstream + 46U));
    header.biClrImportant  = *((uint32_t*) (imstream + 50U));

    return header;
}

static inline BITMAP_PIXEL_ORDERING __cdecl get_pixel_order(_In_ const BITMAPINFOHEADER* const restrict header) {
    return (header->biHeight >= 0) ? BOTTOMUP : TOPDOWN;
}

// reads in a bmp file from disk and deserializes it into a bitmap_t struct
static inline bitmap_t __cdecl bitmap_read(_In_ const wchar_t* const restrict filepath) {
    unsigned size               = 0;
    bitmap_t image              = { 0 }; // will be used as an empty placeholder for premature returns until members are properly assigned

    const uint8_t* const buffer = open(filepath, &size);
    if (!buffer) return image; // open will do the error reporting, so just exiting the function is enough

    const BITMAPFILEHEADER fhead = parse_fileheader(buffer, size);
    if (!fhead.bfSize) return image;
    // again parse_fileheader will report errors and free the buffer, if the predicate isn't satisified, just exit the routine

    const BITMAPINFOHEADER infhead = parse_infoheader(buffer, size);
    if (!infhead.biSize) return image; // error reporting and resource cleanup are handled by parse_infoheader

    image._fileheader = fhead;
    image._infoheader = infhead;
    image._buffer     = buffer;
    image._pixels     = (RGBQUAD*) (buffer + 54);

    return image;
}

// use this to cleanup a bitmap_t after its use
static inline void __cdecl bitmap_close(_In_ bitmap_t* const restrict image) {
    free(image->_buffer);
    memset(image, 0U, sizeof(bitmap_t));
}
