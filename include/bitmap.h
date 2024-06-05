#pragma once

#include <assert.h>
#include <stdint.h>
#include <utilities.h>

// a struct representing a BMP image
typedef struct bitmap {
        BITMAPFILEHEADER _fileheader;
        BITMAPINFOHEADER _infoheader;
        RGBQUAD*         _pixels; // this points to the start of pixels in the file buffer i.e (buffer + 54)
        uint8_t*         _buffer; // this will point to the original file buffer, this is the one that needs deallocation!
} bitmap_t;

// order of pixels in the BMP buffer.
typedef enum { TOPDOWN, BOTTOMUP } BMPPIXDATAORDERING;

// types of compressions used in BMP files.
typedef enum { RGB, RLE8, RLE4, BITFIELDS, UNKNOWN } COMPRESSIONKIND;

// BMP files store this tag as 'B', followed by 'M', i.e 0x424D as an unsigned 16 bit integer,
// when we dereference this 16 bits as an unsigned 16 bit integer on LE machines, the byte order will get swapped i.e the two bytes will be read as 'M', 'B'
const unsigned short start_tag_be = L'B' << 8 | L'M';
const unsigned short start_tag_le = L'M' << 8 | L'B';

static BITMAPFILEHEADER __stdcall parse_fileheader(_In_ const uint8_t* const restrict imstream, _In_ const unsigned size) {
    assert(size >= sizeof(BITMAPFILEHEADER));
    BITMAPFILEHEADER header = { .bfType = 0, .bfSize = 0, .bfReserved1 = 0, .bfReserved2 = 0, .bfOffBits = 0 };

    if (*((uint16_t*) (imstream)) != start_tag_le) {
        fputws(L"Error in parsefileheader, file appears not to be a Windows BMP file\n", stderr);
        return header;
    }

    header.bfSize    = *(uint32_t*) (imstream + 2);
    header.bfOffBits = *(uint32_t*) (imstream + 10);
    return header;
}

static inline BITMAPINFOHEADER __stdcall parse_infoheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) {
    assert(size >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));
    BITMAPINFOHEADER header = { 0 };

    if (*((uint32_t*) (imstream + 14U)) > 40U) {
        fputws(L"Error in parseinfoheader, BMP image seems to contain an unparsable file info header", stderr);
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

static inline BMPPIXDATAORDERING get_pixel_order(_In_ const BITMAPINFOHEADER* const restrict header) {
    return (header->biHeight >= 0) ? BOTTOMUP : TOPDOWN;
}

bitmap_t bitmap_read(_In_ const wchar_t* const restrict filepath) {
    unsigned size               = 0;
    bitmap_t image              = { 0 }; // will be used as an empty placeholder for premature returns until members are properly assigned

    const uint8_t* const buffer = open(filepath, &size);
    if (!buffer) return image; // open will do the error reporting, so just exiting the function is enough

    const BITMAPFILEHEADER fhead = parse_fileheader(buffer, size); // 14 bytes (packed)
    if (!fhead.bfSize) return image; // again parse_fileheader will report errors, if the predicate isn't satisified, exit the routine

    const BITMAPINFOHEADER infhead = parse_infoheader(buffer, size); // 40 bytes (packed)
    if (!infhead.biSize) return image;                               // error reporting is handled by parse_infoheader

    // creating and using a new buffer to only store pixels sounds like a clean idea but it brings a string of performance issues
    // 1) an additional heap allocation for the new buffer and deallocation of the original buffer
    // 2) now that the buffer only holds pixels, we'll need to serialize the structs separately when serializing the image
    // either constructing a temporary array of 54 bytes that hold the structs, and calling CreateFileW with CREATE_NEW first to serialize the
    // structs, closing that file and then reopening it using CreateFileW with OPEN_EXISTING and FILE_APPEND_DATA to append the pixel buffer
    // paying the penalty for two IO calls.
    // 3) or we could create a new buffer with enough space for the structs and pixels and then copy the structs and pixels there first,
    // followed by serialization of this new buffer with one call to CreateFileW, the caveat here is a gratuitous allocation and deallocation
    // const size_t npixels = (size - 54) / 4; // RGBQUAD consumes 4 bytes

    // const HANDLE64 hProcHeap = GetProcessHeap();
    // if (hProcHeap == INVALID_HANDLE_VALUE) {
    //     fwprintf_s(stderr, L"Error %lu in GetProcessHeap\n", GetLastError());
    //     return image;
    // }

    // uint8_t*     pixels  = NULL;
    // if (!(pixels = HeapAlloc(hProcHeap, 0, size - 54))) { }
    // even though bmp_t's `pixels` member is declared as an array of RGBQUADs, we will not be creating RGBQUADs before writing them to the buffer.
    // the compiler may choose to optimize this away but performance wise this is too hefty a price to pay.
    // copying the raw bytes will make no difference granted that we dereference the pixels buffer at appropriate 4 byte intervals as RGBQUADs.

    // if stuff goes left, memcpy_s will raise an access violation exception, not bothering error handling here.
    // memcpy_s(pixels, size - 54, buffer + 54, size - 54);
    image.fileheader = fhead;
    image.infoheader = infhead;
    image.buffer     = buffer;
    image.pixels     = (RGBQUAD*) (buffer + 54);
    // HeapFree(hProcHeap, 0, buffer); // loose the raw bytes buffer

    return image;
}

// prints out information about the passed BMP file
void bitmap_info(_In_ const bitmap_t* const restrict image) {
    wprintf_s(
        L"|---------------------------------------------------------------------------|"
        L"%15s bitmap image (%3.4Lf MiBs)\n"
        L"Pixel ordering: %10s\n"
        L"Width: %5lu pixels, Height: %5lu pixels\n"
        L"Bit depth: %3u\n"
        L"Resolution (PPM): X {%5ld} Y {%5ld}\n"
        L"|---------------------------------------------------------------------------|",
        image->infoheader.biSizeImage ? L"Compressed" : L"Uncompressed",
        image->fileheader.bfSize / (1024.0L * 1024.0L),
        get_pixel_order(&image->infoheader) == BOTTOMUP ? L"bottom-up" : L"top-down",
        image->infoheader.biWidth,
        image->infoheader.biHeight,
        image->infoheader.biBitCount,
        image->infoheader.biXPelsPerMeter,
        image->infoheader.biYPelsPerMeter
    );

    if (image->infoheader.biSizeImage) { // don't bother if the image isn't compressed
        switch (image->infoheader.biCompression) {
            case RGB       : _putws(L"RGB"); break;
            case RLE4      : _putws(L"RLE4"); break;
            case RLE8      : _putws(L"RLE8"); break;
            case BITFIELDS : _putws(L"BITFIELDS"); break;
            case UNKNOWN   : _putws(L"UNKNOWN"); break;
            default        : break;
        }
    }
}
