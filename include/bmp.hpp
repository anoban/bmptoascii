#pragma once

// clang-format off
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_MEAN
#define NOMINMAX

#include <array> // NOLINT(unused-includes)
#include <cassert>
#include <cmath> // NOLINT(unused-includes)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <numeric>

#include <Windows.h>
// clang-format on

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

namespace bmp {
    [[nodiscard("Expensive")]] static inline std::vector<uint8_t> OpenImage(
        _In_ const wchar_t* const filename, _Out_ uint64_t* const nread_bytes
    ) {
        *nread_bytes = 0;
        void *handle = nullptr, *buffer = nullptr;
        DWORD nbytes = 0;

        handle       = ::CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);

        if (handle != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER file_size;
            if (!GetFileSizeEx(handle, &file_size)) {
                fwprintf_s(stderr, L"Error %lu in GetFileSizeEx\n", GetLastError());
                return nullptr;
            }

            // caller is responsible for freeing this buffer.
            buffer = malloc(file_size.QuadPart);
            if (buffer) {
                if (ReadFile(handle, buffer, file_size.QuadPart, &nbytes, nullptr)) {
                    *nread_bytes = nbytes;
                    return buffer;

                } else {
                    fwprintf_s(stderr, L"Error %lu in ReadFile\n", GetLastError());
                    CloseHandle(handle);
                    free(buffer);
                    return nullptr;
                }

            } else {
                fputws(L"Memory allocation error: malloc returned nullptr", stderr);
                CloseHandle(handle);
                return nullptr;
            }

        } else {
            fwprintf_s(stderr, L"Error %lu in CreateFileW\n", GetLastError());
            return nullptr;
        }
    }

    static constexpr BITMAPFILEHEADER __ParseBitmapFileHeader(_In_ const std::vector<uint8_t>& imstream) noexcept {
        assert(imstream.size() >= sizeof(BITMAPFILEHEADER));

        BITMAPFILEHEADER header { .bfType = 0, .bfSize = 0, .bfReserved1 = 0, .bfReserved2 = 0, .bfOffBits = 0 };

        header.bfType = (((uint16_t) (*(imstream.data() + 1))) << 8) | ((uint16_t) (*imstream.data()));
        if (header.bfType != (((uint16_t) 'M' << 8) | (uint16_t) 'B')) {
            fputws(L"Error in __ParseBitmapFileHeader, file appears not to be a Windows BMP file\n", stderr);
            return header;
        }

        header.bfSize    = *reinterpret_cast<const uint32_t*>(imstream.data() + 2);
        header.bfOffBits = *reinterpret_cast<const uint32_t*>(imstream.data() + 10);

        return header;
    }

    static constexpr BITMAPINFOHEADER __ParseBitmapInfoHeader(_In_ const std::vector<uint8_t>& imstream) noexcept {
        assert(imstream.size() >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));

        BITMAPINFOHEADER header {};

        if (*((uint32_t*) (imstream.data() + 14U)) > 40) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            fputws(L"BMP image seems to contain an unparsable file info header", stderr);
            return header;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        header.biSize          = *(reinterpret_cast<const uint32_t*>(imstream.data() + 14U));
        header.biWidth         = *(reinterpret_cast<const uint32_t*>(imstream.data() + 18U));
        header.biHeight        = *(reinterpret_cast<const int32_t*>(imstream.data() + 22U));
        header.biPlanes        = *(reinterpret_cast<const uint16_t*>(imstream.data() + 26U));
        header.biBitCount      = *(reinterpret_cast<const uint16_t*>(imstream.data() + 28U));
        header.biCompression   = *(reinterpret_cast<const uint32_t*>(imstream.data() + 30U));
        header.biSizeImage     = *(reinterpret_cast<const uint32_t*>(imstream.data() + 34U));
        header.biXPelsPerMeter = *(reinterpret_cast<const uint32_t*>(imstream.data() + 38U));
        header.biYPelsPerMeter = *(reinterpret_cast<const uint32_t*>(imstream.data() + 42U));
        header.biClrUsed       = *(reinterpret_cast<const uint32_t*>(imstream.data() + 46U));
        header.biClrImportant  = *(reinterpret_cast<const uint32_t*>(imstream.data() + 50U));
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        return header;
    }

    // A struct representing a BMP image.

    template<typename T, bool = std::is_unsigned<T>::value> class bmp;

    template<typename T> class bmp<T, true> final { // partial specialization that only supports unsigned scalar types
        public:
            using size_type = T;

        private:
            size_type        fsize;
            size_type        npixels;
            BITMAPFILEHEADER fhead;
            BITMAPINFOHEADER infhead;
            RGBQUAD*         pixel_buffer;

        public:
            inline bmp(_In_ const uint8_t* const imstream /* will be freed by this procedure */, _In_ const size_t size) {
                bmp image = {
                    .fsize        = 0,
                    .npixels      = 0,
                    .fhead        = { 0, 0, 0, 0, 0 },
                    .infhead      = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    .pixel_buffer = nullptr
                };

                if (!imstream) {
                    fwprintf_s(stderr, L"Error in %s @ line %d: nullptr buffer received!\n", __FUNCTIONW__, __LINE__);
                    return image;
                }

                const BITMAPFILEHEADER fh   = __ParseBitmapFileHeader(imstream, size);
                const BITMAPINFOHEADER infh = __ParseBitmapInfoHeader(imstream, size);

                RGBQUAD* const buffer       = malloc(size - 54);
                if (buffer) {
                    memcpy_s(buffer, size - 54, imstream + 54, size - 54);
                } else {
                    fwprintf_s(stderr, L"Error in %s @ line %d: malloc falied!\n", __FUNCTIONW__, __LINE__);
                    return image;
                }

                return (bmp) { .fsize = size, .npixels = (size - 54) / 4, .fhead = fh, .infhead = infh, .pixel_buffer = buffer };
            }

            inline void BmpInfo(_In_ const bmp* const image) const noexcept {
                wprintf_s(
                    L"\nFile size %Lf MiBs\nPixel data start offset: %d\n"
                    L"BITMAPINFOHEADER size: %u\nImage width: %u\nImage height: %d\nNumber of planes: %hu\n"
                    L"Number of bits per pixel: %hu\nImage size: %u\nResolution PPM(X): %u\nResolution PPM(Y): %u\nNumber of used colormap entries: %d\n"
                    L"Number of important colors: %d\n",
                    (long double) (image->fhead.bfSize) / (1024LLU * 1024LLU),
                    image->fhead.bfOffBits,
                    image->infhead.biSize,
                    image->infhead.biWidth,
                    image->infhead.biHeight,
                    image->infhead.biPlanes,
                    image->infhead.biBitCount,
                    image->infhead.biSizeImage,
                    image->infhead.biXPelsPerMeter,
                    image->infhead.biYPelsPerMeter,
                    image->infhead.biClrUsed,
                    image->infhead.biClrImportant
                );

                switch (image->infhead.biCompression) {
                    case 0  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RGB"); break;
                    case 1  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RLE4"); break;
                    case 2  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RLE8"); break;
                    case 3  : _putws(L"BITMAPINFOHEADER.CMPTYPE: BITFIELDS"); break;
                    default : _putws(L"BITMAPINFOHEADER.CMPTYPE: UNKNOWN"); break;
                }

                wprintf_s(
                    L"%s BMP file\n"
                    L"BMP pixel ordering: %s\n",
                    image->infhead.biSizeImage != 0 ? L"Compressed" : L"Uncompressed",
                    image->infhead.biHeight >= 0 ? L"BOTTOMUP\n" : L"TOPDOWN\n"
                );

                return;
            }
    };

} // namespace bmp
