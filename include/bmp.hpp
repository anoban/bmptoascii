#pragma once

// clang-format off
#include <wingdi.h>
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
#include <optional>
#include <string>
#include <Windows.h>
// clang-format on

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

namespace bmp {
    [[nodiscard("Expensive")]] static inline std::optional<std::vector<uint8_t>> Open(_In_ const wchar_t* const filename) {
        DWORD          nbytes {};
        LARGE_INTEGER  liFsize = { .QuadPart = 0LLU };
        const HANDLE64 hFile   = ::CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            ::fwprintf_s(stderr, L"Error %lu in CreateFileW\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
            goto INVALID_HANDLE_ERR;
        }

        if (!::GetFileSizeEx(hFile, &liFsize)) {
            ::fwprintf_s(stderr, L"Error %lu in GetFileSizeEx\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
            goto GET_FILESIZE_ERR;
        }

        std::vector<uint8_t> buffer(liFsize.QuadPart);

        if (!::ReadFile(hFile, buffer.data(), liFsize.QuadPart, &nbytes, NULL)) {
            ::fwprintf_s(stderr, L"Error %lu in ReadFile\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
            goto READFILE_ERR;
        }

        ::CloseHandle(hFile);
        return buffer;

GET_FILESIZE_ERR:
        CloseHandle(hFile);
INVALID_HANDLE_ERR:
        return std::nullopt;
    }

    static inline BITMAPFILEHEADER __ParseBitmapFileHeader(_In_ const std::vector<uint8_t>& imstream) noexcept {
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

    static inline BITMAPINFOHEADER __ParseBitmapInfoHeader(_In_ const std::vector<uint8_t>& imstream) noexcept {
        assert(imstream.size() >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));

        BITMAPINFOHEADER header {};

        if (*reinterpret_cast<const uint32_t*>(imstream.data() + 14U) > 40U) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
            size_type        _fsize;
            size_type        _npixels;
            BITMAPFILEHEADER _fhead;
            BITMAPINFOHEADER _infhead;
            RGBQUAD*         _pixel_buffer;

        public:
            constexpr bmp() noexcept : _fsize(), _npixels(), _fhead(), _infhead(), _pixel_buffer() { }

            constexpr bmp(_In_ const std::vector<uint8_t>& imstream) noexcept {
                bmp image = {
                    .fsize        = 0,
                    .npixels      = 0,
                    .fhead        = { 0, 0, 0, 0, 0 },
                    .infhead      = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    .pixel_buffer = nullptr
                };

                if (imstream.empty()) {
                    ::fwprintf_s(stderr, L"Error in %s @ line %d: nullptr buffer received!\n", __FUNCTIONW__, __LINE__);
                    return image;
                }

                const BITMAPFILEHEADER fh   = __ParseBitmapFileHeader(imstream, size);
                const BITMAPINFOHEADER infh = __ParseBitmapInfoHeader(imstream, size);

                RGBQUAD* const buffer       = malloc(size - 54);
                if (buffer) {
                    memcpy_s(buffer, size - 54, imstream + 54, size - 54);
                } else {
                    ::fwprintf_s(stderr, L"Error in %s @ line %d: malloc falied!\n", __FUNCTIONW__, __LINE__);
                    return image;
                }

                return (bmp) { .fsize = size, .npixels = (size - 54) / 4, .fhead = fh, .infhead = infh, .pixel_buffer = buffer };
            }

            constexpr bmp(const bmp& other) noexcept :
                _fsize(other._fsize),
                _npixels(other._npixels),
                _fhead(other._fhead),
                _infhead(other._infhead),
                _pixel_buffer(new RGBQUAD[other._npixels]) {
                std::copy();
            }

            constexpr bmp(bmp&& other) noexcept { }

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
