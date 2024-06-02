#pragma once

// clang-format off
#define _AMD64_
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_MEAN
#define NOMINMAX
#include <windef.h>
#include <wingdi.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
// clang-format on

#include <cassert>
#include <cstdint>
#include <optional>

#include <utilities.hpp>

static_assert(sizeof(BITMAPINFOHEADER) == 40LLU, "BITMAPINFOHEADER is expected to be 40 bytes in size, but is not so!");
static_assert(sizeof(BITMAPFILEHEADER) == 14LLU, "BITMAPFILEHEADER is expected to be 14 bytes in size, but is not so!");

[[nodiscard("expensive")]] static inline std::optional<uint8_t*> open(
    _In_ const wchar_t* const filename, _Inout_ unsigned* const rbytes
) noexcept {
    DWORD          nbytes {};
    LARGE_INTEGER  liFsize { .QuadPart = 0LLU };
    const HANDLE64 hFile { ::CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr) };
    uint8_t*       buffer {};
    BOOL           bReadStatus {};

    if (hFile == INVALID_HANDLE_VALUE) {
        ::fwprintf_s(stderr, L"Error %lu in CreateFileW\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
        goto INVALID_HANDLE_ERR;
    }

    if (!::GetFileSizeEx(hFile, &liFsize)) {                                     // NOLINT(readability-implicit-bool-conversion)
        ::fwprintf_s(stderr, L"Error %lu in GetFileSizeEx\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
        goto GET_FILESIZE_ERR;
    }

    buffer = new (std::nothrow) uint8_t[liFsize.QuadPart];
    if (!buffer) {                                                         // NOLINT(readability-implicit-bool-conversion)
        ::fputws(L"Memory allocation error in utilities::open\n", stderr); // NOLINT(cppcoreguidelines-pro-type-vararg)
        goto GET_FILESIZE_ERR;
    }

    bReadStatus = ::ReadFile(hFile, buffer, liFsize.QuadPart, &nbytes, nullptr);
    if (!bReadStatus) {                                                     // NOLINT(readability-implicit-bool-conversion)
        ::fwprintf_s(stderr, L"Error %lu in ReadFile\n", ::GetLastError()); // NOLINT(cppcoreguidelines-pro-type-vararg)
        goto GET_FILESIZE_ERR;
    }

    ::CloseHandle(hFile);
    *rbytes = nbytes;
    return buffer;

GET_FILESIZE_ERR:
    delete[] buffer;
    ::CloseHandle(hFile);
INVALID_HANDLE_ERR:
    *rbytes = 0;
    return std::nullopt;
}

namespace bmp {

    // BMP files store this tag as 'B', followed by 'M', i.e 0x424D as an unsigned 16 bit integer,
    // when we dereference this 16 bits as an unsigned 16 bit integer on LE machines, the byte order will get swapped i.e the two bytes will be read as 'M', 'B'
    constexpr unsigned short start_tag_be { L'B' << 8 | L'M' };
    constexpr unsigned short start_tag_le { L'M' << 8 | L'B' };

    static inline BITMAPFILEHEADER __stdcall parsefileheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) noexcept {
        assert(size >= sizeof(BITMAPFILEHEADER));
        BITMAPFILEHEADER header { .bfType = 0, .bfSize = 0, .bfReserved1 = 0, .bfReserved2 = 0, .bfOffBits = 0 };

        if (*reinterpret_cast<const uint16_t*>(imstream) != start_tag_le) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            fputws(L"Error in parsefileheader, file appears not to be a Windows BMP file\n", stderr);
            return header;
        }

        header.bfSize    = *reinterpret_cast<const uint32_t*>(imstream + 2);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        header.bfOffBits = *reinterpret_cast<const uint32_t*>(imstream + 10); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return header;
    }

    static inline BITMAPINFOHEADER __stdcall parseinfoheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) noexcept {
        assert(size >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));
        BITMAPINFOHEADER header {};

        if (*reinterpret_cast<const uint32_t*>(imstream + 14U) > 40U) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            fputws(L"Error in parseinfoheader, BMP image seems to contain an unparsable file info header", stderr);
            return header;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        header.biSize          = *(reinterpret_cast<const uint32_t*>(imstream + 14U));
        header.biWidth         = *(reinterpret_cast<const uint32_t*>(imstream + 18U)); // NOLINT(cppcoreguidelines-narrowing-conversions)
        header.biHeight        = *(reinterpret_cast<const int32_t*>(imstream + 22U));
        header.biPlanes        = *(reinterpret_cast<const uint16_t*>(imstream + 26U));
        header.biBitCount      = *(reinterpret_cast<const uint16_t*>(imstream + 28U));
        header.biCompression   = *(reinterpret_cast<const uint32_t*>(imstream + 30U));
        header.biSizeImage     = *(reinterpret_cast<const uint32_t*>(imstream + 34U));
        header.biXPelsPerMeter = *(reinterpret_cast<const uint32_t*>(imstream + 38U)); // NOLINT(cppcoreguidelines-narrowing-conversions)
        header.biYPelsPerMeter = *(reinterpret_cast<const uint32_t*>(imstream + 42U)); // NOLINT(cppcoreguidelines-narrowing-conversions)
        header.biClrUsed       = *(reinterpret_cast<const uint32_t*>(imstream + 46U));
        header.biClrImportant  = *(reinterpret_cast<const uint32_t*>(imstream + 50U));
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return header;
    }

    template<typename pixel_type> requires ::is_rgb<pixel_type> class bmp final { // class representing a Windows BMP image.
        public:
            using size_type       = unsigned long long;
            using value_type      = std::remove_cv_t<pixel_type>;
            using pointer         = std::remove_cv_t<pixel_type>*;
            using const_pointer   = const std::remove_cv_t<pixel_type>*;
            using reference       = std::remove_cv_t<pixel_type>&;
            using const_reference = const std::remove_cv_t<pixel_type>&;
            using difference_type = ptrdiff_t;
            using iterator        = iterator::random_access_iterator<value_type>;
            using const_iterator  = iterator::template random_access_iterator<const value_type>;

        private:
            size_type        _fsize;        // file size on disk
            BITMAPFILEHEADER _fhead;        // file header
            BITMAPINFOHEADER _infhead;      // information header
            size_type        _npixels;      // number of pixels in the image
            uint8_t*         _raw_buffer;   // raw bytes of the image file
            pointer          _pixel_buffer; //  _raw_buffer + 54; type casted to pointer type
                                            // could opt for using a separate buffer, but wasteful and will bite runtime performance

        public:
            constexpr bmp() noexcept : _fsize(), _fhead(), _infhead(), _npixels(), _raw_buffer(), _pixel_buffer() { }

            constexpr bmp(_In_reads_(len) const uint8_t* const imstream, _In_ const size_type len) noexcept :
                _fsize(len), _fhead(parsefileheader(imstream, len)), _infhead(parseinfoheader(imstream, len)) {
                if (!imstream) { // NOLINT(readability-implicit-bool-conversion)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    ::fwprintf_s(stderr, L"Error in %s @ line %d: empty buffer received!\n", __FUNCTIONW__, __LINE__);
                    return;
                }
            }

            constexpr bmp(const bmp& other) noexcept :
                _fsize(other._fsize),
                _npixels(other._npixels),
                _fhead(other._fhead),
                _infhead(other._infhead),
                _raw_buffer(new (std::nothrow) uint8_t[other._fsize]) {
                // handle pixel buffer and raw buffer
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                ::fwprintf_s(stderr, L"Error in %s @ line %d: empty buffer received!\n", __FUNCTIONW__, __LINE__);
                std::copy();
            }

            constexpr bmp(bmp&& other) noexcept :
                _fsize(other._fsize),
                _npixels(other._npixels),
                _fhead(other._fhead),
                _infhead(other._infhead),
                _pixel_buffer(other._pixel_buffer),
                _raw_buffer(other._raw_buffer) {
                // cleanup the object stolen from
                other._fsize = other._npixels = 0;
                other._pixel_buffer           = nullptr;
                other._raw_buffer             = nullptr;
                other._fhead                  = BITMAPFILEHEADER {};
                other._infhead                = BITMAPINFOHEADER {};
            }

            constexpr bmp& operator=(const bmp& other) noexcept {
                if (this == &other) return *this;
                _fsize        = other._fsize;
                _npixels      = other._npixels;
                _fhead        = other._fhead;
                _infhead      = other._infhead;

                // handle the memory allocation
                _pixel_buffer = other._pixel_buffer;
                _raw_buffer   = other._raw_buffer;
                return *this;
            }

            constexpr bmp& operator=(bmp&& other) noexcept {
                if (this == &other) return *this;
                _fsize        = other._fsize;
                _npixels      = other._npixels;
                _fhead        = other._fhead;
                _infhead      = other._infhead;
                _pixel_buffer = other._pixel_buffer;
                _raw_buffer   = other._raw_buffer;

                // cleanup the object stolen from
                other._fsize = other._npixels = 0;
                other._pixel_buffer           = nullptr;
                other._raw_buffer             = nullptr;
                other._fhead                  = BITMAPFILEHEADER {};
                other._infhead                = BITMAPINFOHEADER {};

                return *this;
            }

            ~bmp() noexcept {
                delete[] _raw_buffer;
                _pixel_buffer = nullptr;
                _raw_buffer   = nullptr;
                _fsize = _npixels = 0;
                _fhead            = BITMAPFILEHEADER {};
                _infhead          = BITMAPINFOHEADER {};
            }

            constexpr size_type width() const noexcept { return _infhead.biWidth; }

            constexpr size_type height() const noexcept { return _infhead.biHeight; }

            void info() const noexcept {
                // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)

                ::wprintf_s(
                    L"\nFile size %Lf MiBs\nPixel data start offset: %d\n"
                    L"BITMAPINFOHEADER size: %u\nImage width: %u\nImage height: %d\nNumber of planes: %hu\n"
                    L"Number of bits per pixel: %hu\nImage size: %u\nResolution PPM(X): %u\nResolution PPM(Y): %u\nNumber of used colormap entries: %d\n"
                    L"Number of important colors: %d\n",
                    _fhead.bfSize / 1048576.000L, // 1024 * 1024
                    _fhead.bfOffBits,
                    _infhead.biSize,
                    _infhead.biWidth,
                    _infhead.biHeight,
                    _infhead.biPlanes,
                    _infhead.biBitCount,
                    _infhead.biSizeImage,
                    _infhead.biXPelsPerMeter,
                    _infhead.biYPelsPerMeter,
                    _infhead.biClrUsed,
                    _infhead.biClrImportant
                );

                switch (_infhead.biCompression) {
                    case 0  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RGB"); break;
                    case 1  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RLE4"); break;
                    case 2  : _putws(L"BITMAPINFOHEADER.CMPTYPE: RLE8"); break;
                    case 3  : _putws(L"BITMAPINFOHEADER.CMPTYPE: BITFIELDS"); break;
                    default : _putws(L"BITMAPINFOHEADER.CMPTYPE: UNKNOWN"); break;
                }

                ::wprintf_s(
                    L"%s BMP file\n"
                    L"BMP pixel ordering: %s\n",
                    _infhead.biSizeImage != 0 ? L"Compressed" : L"Uncompressed",
                    _infhead.biHeight >= 0 ? L"BOTTOMUP\n" : L"TOPDOWN\n"
                );

                // NOLINTEND(cppcoreguidelines-pro-type-vararg)
            }

            constexpr iterator begin() noexcept { }

            constexpr const_iterator begin() const noexcept { }

            constexpr const_iterator cbegin() const noexcept { }

            constexpr iterator end() noexcept { }

            constexpr const_iterator end() const noexcept { }

            constexpr const_iterator cend() const noexcept { }

            constexpr pointer data() noexcept { }

            constexpr const_pointer data() const noexcept { }

            __forceinline std::wstring __stdcall tostring() noexcept {
                return (image->infhead.biWidth <= 140) ? to_string(image) : to_downscaled_string(image);
            }
    };

} // namespace bmp
