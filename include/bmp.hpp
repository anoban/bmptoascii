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

namespace bmp {
    [[nodiscard("expensive")]] static inline std::optional<uint8_t*> open(
        _In_ const wchar_t* const filename, _Inout_ unsigned* const rbytes
    ) noexcept;

    static inline BITMAPFILEHEADER __stdcall parsefileheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) noexcept;

    static inline BITMAPINFOHEADER __stdcall parseinfoheader(_In_ const uint8_t* const imstream, _In_ const unsigned size) noexcept;

    template<typename T = RGBQUAD> requires utilities::is_rgb<T> class bmp;
} // namespace bmp
