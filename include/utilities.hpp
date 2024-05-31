#pragma once

// clang-format off
#define _AMD64_
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_MEAN
#define NOMINMAX
#include <windef.h>
#include <wingdi.h>
// clang-format on

#include <array>
#include <iterator>
#include <limits>
#include <type_traits>

#include <bmp.hpp>

namespace utilities {
    // should be the wingdi provided RGBTRIPLE or RGBQUAD structs or should have unsigned 8 bit data members named rgbBlue, rgbGreen and rgbRed
    template<typename T> concept is_rgb = std::is_same<typename std::remove_cv<T>::type, RGBQUAD>::value ||
                                          std::is_same<typename std::remove_cv<T>::type, RGBTRIPLE>::value ||
                                          requires(const typename std::remove_cv<T>::type& pixel) {
                                              requires(std::numeric_limits<decltype(pixel.rgbBlue)>::max() == UCHAR_MAX);
                                              pixel.rgbBlue + pixel.rgbGreen + pixel.rgbRed; // members must support the operator+
                                              pixel.rgbBlue * 0.75673L; // members must also support operator* with floating point types
                                          };

    namespace palettes {
        // ASCII characters in ascending order of luminance
        static constexpr std::array<wchar_t, 27> palette_minimal { L'_', L'.', L',', L'-', L'=', L'+', L':', L';', L'c',
                                                                   L'b', L'a', L'!', L'?', L'1', L'2', L'3', L'4', L'5',
                                                                   L'6', L'7', L'8', L'9', L'$', L'W', L'#', L'@', L'N' };

        static constexpr std::array<wchar_t, 44> palette { L' ', L'.', L'-', L',', L':', L'+', L'~', L';', L'(', L'%', L'x',
                                                           L'1', L'*', L'n', L'u', L'T', L'3', L'J', L'5', L'$', L'S', L'4',
                                                           L'F', L'P', L'G', L'O', L'V', L'X', L'E', L'Z', L'8', L'A', L'U',
                                                           L'D', L'H', L'K', L'W', L'@', L'B', L'Q', L'#', L'0', L'M', L'N' };

        static constexpr std::array<wchar_t, 70> palette_extended {
            L' ', L'.', L'\'', L'`', L'^', L'"', L',', L':', L';', L'I', L'l',  L'!', L'i', L'>', L'<', L'~', L'+', L'_',
            L'-', L'?', L']',  L'[', L'}', L'{', L'1', L')', L'(', L'|', L'\\', L'/', L't', L'f', L'j', L'r', L'x', L'n',
            L'u', L'v', L'c',  L'z', L'X', L'Y', L'U', L'J', L'C', L'L', L'Q',  L'0', L'O', L'Z', L'm', L'w', L'q', L'p',
            L'd', L'b', L'k',  L'h', L'a', L'o', L'*', L'#', L'M', L'W', L'&',  L'8', L'%', L'B', L'@', L'$'
        };

    } // namespace palettes

    namespace transformers {

        // a functor giving back the arithmetic average of an RGB pixel values
        template<
            typename pixel_type  = RGBQUAD,
            typename return_type = unsigned,
            typename             = std::enable_if<std::is_unsigned_v<return_type>, bool>::type>
        requires is_rgb<pixel_type> struct arithmetic_average;

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned> struct weighted_average;

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned> struct minmax_average;

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned> struct luminosity;

    } // namespace transformers

    // a composer that uses a specified pair of palette and RGB to BW mapper to return an appropriate wchar_t
    template<
        typename transformer_type = transformers::weighted_average<RGBQUAD>, // default RGB transformer
        unsigned plength /* palette length */ =
            palettes::palette_extended.size()> // the palette used by the mapper will default to palette_extended
    // imposing class level type constraints won't play well with the user defined default ctor as the compiler cannot deduce the types used in the requires expressions
    // hence, moving them to the custom ctor.
    class rgbmapper;

    template<typename T> class random_access_iterator final { // unchecked iterator class to be used with class bmp
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = std::remove_cv_t<T>;
            using size_type         = unsigned long long;
            using difference_type   = ptrdiff_t;
            using pointer           = T*;
            using const_pointer     = const T*;
            using reference         = T&;
            using const_reference   = const T&;

        private:
            pointer   _resource;
            size_type _offset;
            size_type _length;

        public:
            constexpr random_access_iterator() noexcept;

            constexpr random_access_iterator(pointer _ptr, size_type _size) noexcept;

            constexpr random_access_iterator(pointer _ptr, size_type _pos, size_type _size) noexcept;

            constexpr random_access_iterator(const random_access_iterator& other) noexcept;

            constexpr random_access_iterator(random_access_iterator&& other) noexcept;

            constexpr ~random_access_iterator() noexcept;

            constexpr random_access_iterator& operator=(const random_access_iterator& other) noexcept;

            constexpr random_access_iterator& operator=(random_access_iterator&& other) noexcept;

            constexpr random_access_iterator& operator++() noexcept;

            constexpr random_access_iterator operator++(int) noexcept;

            constexpr random_access_iterator& operator--() noexcept;

            constexpr random_access_iterator operator--(int) noexcept;

            constexpr reference operator*() noexcept;

            constexpr const_reference operator*() const noexcept;
    };

} // namespace utilities
