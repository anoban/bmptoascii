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

template<typename T, bool = std::is_same_v<T, RGBQUAD> || std::is_same_v<T, RGBTRIPLE>> struct is_wingdi;

template<typename T> struct is_wingdi<T, true> {
        using type = std::remove_cv_t<T>;
        static constexpr bool value { true };
};

template<typename T> inline constexpr bool is_wingdi_v = is_wingdi<T>::value;
template<typename T> using is_wingdi_t                 = typename is_wingdi<T>::type;

// should be the wingdi provided RGBTRIPLE or RGBQUAD structs or should have unsigned 8 bit data members named rgbBlue, rgbGreen and rgbRed
template<typename T> concept is_rgb =
    std::is_same<typename std::remove_cv<T>::type, RGBQUAD>::value || std::is_same<typename std::remove_cv<T>::type, RGBTRIPLE>::value ||
    requires(const typename std::remove_cv<T>::type& pixel) {
        requires(std::numeric_limits<decltype(pixel.rgbBlue)>::max() == std::numeric_limits<unsigned char>::max());
        pixel.rgbBlue + pixel.rgbGreen + pixel.rgbRed; // members must support the operator+
        pixel.rgbBlue * 0.75673L;                      // members must also support operator* with floating point types
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

    static constexpr std::array<wchar_t, 70> palette_extended { L' ', L'.', L'\'', L'`', L'^',  L'"', L',', L':', L';', L'I', L'l', L'!',
                                                                L'i', L'>', L'<',  L'~', L'+',  L'_', L'-', L'?', L']', L'[', L'}', L'{',
                                                                L'1', L')', L'(',  L'|', L'\\', L'/', L't', L'f', L'j', L'r', L'x', L'n',
                                                                L'u', L'v', L'c',  L'z', L'X',  L'Y', L'U', L'J', L'C', L'L', L'Q', L'0',
                                                                L'O', L'Z', L'm',  L'w', L'q',  L'p', L'd', L'b', L'k', L'h', L'a', L'o',
                                                                L'*', L'#', L'M',  L'W', L'&',  L'8', L'%', L'B', L'@', L'$' };

} // namespace palettes

namespace transformers {

    // a functor giving back the arithmetic average of an RGB pixel values
    template<typename pixel_type = RGBQUAD, typename return_type = unsigned int, bool = std::is_unsigned_v<return_type>>
    requires ::is_rgb<pixel_type> struct arithmetic_average;

    // partial specialization of arithmetic_average for unsigned types
    template<typename pixel_type, typename return_type>
    struct arithmetic_average<
        pixel_type,
        return_type,
        true> { // struct arithmetic_average also doubles as the base class for other functors
            // deriving from arithmetic_average purely to leverage the requires is_rgb<pixel_type> constraint through inheritance with minimal redundancy
            using value_type  = pixel_type;
            using return_type = return_type;

            [[nodiscard]] constexpr return_type operator()(const value_type& pixel) const noexcept {
                // we don't want overflows or truncations here
                return static_cast<return_type>((static_cast<double>(pixel.rgbBlue) + pixel.rgbGreen + pixel.rgbRed) / 3.000);
            }
    };

    template<typename pixel_type = RGBQUAD, typename return_type = unsigned int>
    struct weighted_average final : public arithmetic_average<pixel_type, return_type> {
            // using arithmetic_average<pixel_type, return_type>::value_type makes the signature extremely verbose
            using value_type  = pixel_type;
            using return_type = return_type;

            // weighted average of an RGB pixel values
            [[nodiscard]] constexpr return_type operator()(const value_type& pixel) const noexcept {
                return static_cast<return_type>(pixel.rgbBlue * 0.299L + pixel.rgbGreen * 0.587L + pixel.rgbRed * 0.114L);
            }
    };

    template<typename pixel_type = RGBQUAD, typename return_type = unsigned int>
    struct minmax_average final : public arithmetic_average<pixel_type, return_type> {
            using value_type  = pixel_type;
            using return_type = return_type;

            // average of minimum and maximum RGB values in a pixel
            [[nodiscard]] constexpr return_type operator()(const value_type& pixel) const noexcept {
                // we don't want overflows or truncations here
                return static_cast<return_type>(
                    (static_cast<double>(std::min({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })) +
                     std::max({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })) /
                    2.0000
                );
            }
    };

    template<typename pixel_type = RGBQUAD, typename return_type = unsigned int>
    struct luminosity final : public arithmetic_average<pixel_type, return_type> {
            using value_type  = pixel_type;
            using return_type = return_type;

            // luminosity of an RGB pixel
            [[nodiscard]] constexpr return_type operator()(const value_type& pixel) const noexcept {
                return static_cast<return_type>(pixel.rgbBlue * 0.2126L + pixel.rgbGreen * 0.7152L + pixel.rgbRed * 0.0722L);
            }
    };

} // namespace transformers

namespace utilities {

    // a composer that uses a specified pair of palette and RGB to BW mapper to return an appropriate wchar_t
    template<
        typename transformer_type,
        unsigned plength,
        typename = std::enable_if<::is_rgb<typename transformer_type::pixel_type>, typename transformer_type::value_type>::type>
    class rgbmapper final {
        public:
            using size_type      = typename transformer_type::return_type; // will be used to index into the palette buffer
            using value_type     = wchar_t;                                // return type of operator()
            using converter_type = transformer_type;                       // RGB to BW converter type
            using pixel_type     = typename transformer_type::value_type;

        private:
            converter_type               _rgbtransformer;
            std::array<wchar_t, plength> _palette;
            unsigned                     _palette_len;

        public:
            constexpr rgbmapper() noexcept :
                _rgbtransformer(converter_type {}), _palette(palettes::palette_extended), _palette_len(plength) { }

            constexpr rgbmapper(const std::array<wchar_t, plength>& palette, const converter_type& transformer) noexcept requires requires {
                transformer.operator()(); // rgb transformer must have a valid operator()  defined!
            } : _rgbtransformer(transformer), _palette(palette), _palette_len(palette.size()) { }

            constexpr rgbmapper(const rgbmapper& other) noexcept :
                _rgbtransformer(other.scaler), _palette(other.palette), _palette_len(other.palette.size()) { }

            constexpr rgbmapper(rgbmapper&& other) noexcept :
                _rgbtransformer(std::move(other.scaler)), _palette(std::move(other.palette)), _palette_len(_palette.size()) { }

            constexpr rgbmapper& operator=(const rgbmapper& other) noexcept {
                if (this == &other) return *this;
                _rgbtransformer = other._rgbtransformer;
                _palette        = other._palette;
                _palette_len    = other._palette_len;
                return *this;
            }

            constexpr rgbmapper& operator=(rgbmapper&& other) noexcept {
                if (this == &other) return *this;
                _rgbtransformer = other._rgbtransformer;
                _palette        = other._palette;
                _palette_len    = other._palette_len;
                return *this;
            }

            constexpr ~rgbmapper() = default;

            // NEEDS CORRECTIONS!
            // gives the wchar_t corresponding to the provided RGB pixel
            [[nodiscard]] constexpr value_type operator()(const RGBQUAD& pixel) const noexcept {
                // _rgbtransformer(pixel) can range from 0 to 255
                const auto _offset { _rgbtransformer(pixel) };
                // hence, _offset / static_cast<float>(std::numeric_limits<unsigned char>::max()) can range from 0.0 to 1.0
                return _palette[_offset ? (_offset / static_cast<float>(std::numeric_limits<unsigned char>::max()) * _palette_len) - 1 : 0];
            }
    };

} // namespace utilities

namespace iterator {

    template<typename T> class random_access_iterator final { // unchecked random access iterator class to be used with class bmp
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
            pointer   _resource; // pointer to the resource buffer
            size_type _offset;   // offset of element the iterator is currently pointing to
            size_type _length;   // total length of the buffer

        public:
            constexpr random_access_iterator() noexcept : _resource(), _offset(), _length() { }

            constexpr random_access_iterator(pointer _ptr, size_type _size) noexcept : _resource(_ptr), _offset(), _length(_size) { }

            constexpr random_access_iterator(pointer _ptr, size_type _size, size_type _pos) noexcept
                : // need this for .end() and .cend() calls
                _resource(_ptr), _offset(_pos), _length(_size) { }

            constexpr random_access_iterator(const random_access_iterator& other) noexcept :
                _resource(other._resource), _offset(other._offset), _length(other._length) { }

            constexpr random_access_iterator(random_access_iterator&& other) noexcept :
                _resource(other._resource), _offset(other._offset), _length(other._length) {
                // cleanup the moved object
                other._resource = nullptr;
                other._offset = other._length = 0;
            }

            constexpr ~random_access_iterator() noexcept {
                _resource = nullptr;
                _offset = _length = 0;
            }

            constexpr random_access_iterator& operator=(const random_access_iterator& other) noexcept {
                if (this == &other) return *this;
                _resource = other._resource;
                _offset   = other._offset;
                _length   = other._length;
                return *this;
            }

            constexpr random_access_iterator& operator=(random_access_iterator&& other) noexcept {
                if (this == &other) return *this;
                _resource       = other._resource;
                _offset         = other._offset;
                _length         = other._length;

                other._resource = nullptr;
                other._offset = other._length = 0;

                return *this;
            }

            constexpr bool operator==(const random_access_iterator& other) const noexcept {
                return _resource == other._resource && _offset == other._offset;
            }

            constexpr bool operator!=(const random_access_iterator& other) const noexcept {
                return _resource != other._resource || _offset != other._offset;
            }

            constexpr random_access_iterator& operator++() noexcept {
                ++_offset;
                return *this;
            }

            constexpr random_access_iterator operator++(int) noexcept {
                ++_offset;
                return { _resource, _offset - 1, _length };
            }

            constexpr random_access_iterator& operator--() noexcept {
                --_offset;
                return *this;
            }

            constexpr random_access_iterator operator--(int) noexcept {
                --_offset;
                return { _resource, _offset + 1, _length };
            }

            constexpr reference operator*() noexcept { return _resource[_offset]; }

            constexpr const_reference operator*() const noexcept { return _resource[_offset]; }

            constexpr pointer _Unwrapped() noexcept { return _resource; }

            constexpr const_pointer _Unwrapped() const noexcept { return _resource; }
    };

} // namespace iterator
