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
#include <cstdio>
#include <iterator>
#include <string>
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
        requires is_rgb<pixel_type>
        struct arithmetic_average { // struct arithmetic_average also doubles as the base class for other functors
                // deriving from arithmetic_average purely to leverage the requires is_rgb<pixel_type> constraint through inheritance with minimal redundancy
                using value_type = return_type;
                constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(
                        (/* we don't want overflows or truncations here */ static_cast<double>(pixel.rgbBlue) + pixel.rgbGreen +
                         pixel.rgbRed) /
                        3.000L
                    );
                }
        };

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned>
        struct weighted_average final : public arithmetic_average<pixel_type, return_type> {
                // using arithmetic_average<pixel_type, return_type>::value_type makes the signature extremely verbose
                using value_type = return_type;
                // weighted average of an RGB pixel values
                constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(pixel.rgbBlue * 0.299L + pixel.rgbGreen * 0.587L + pixel.rgbRed * 0.114L);
                }
        };

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned>
        struct minmax_average final : public arithmetic_average<pixel_type, return_type> {
                using value_type = return_type;
                // average of minimum and maximum RGB values in a pixel
                constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(
                        (/* we don't want overflows or truncations here */ static_cast<double>(
                             std::min({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })
                         ) +
                         std::max({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })) /
                        2.0000L
                    );
                }
        };

        template<typename pixel_type = RGBQUAD, typename return_type = unsigned>
        struct luminosity final : public arithmetic_average<pixel_type, return_type> {
                using value_type = return_type;
                // luminosity of an RGB pixel
                constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(pixel.rgbBlue * 0.2126L + pixel.rgbGreen * 0.7152L + pixel.rgbRed * 0.0722L);
                }
        };

    } // namespace transformers

    // a composer that uses a specified pair of palette and RGB to BW mapper to return an appropriate wchar_t
    template<
        typename transformer_type = transformers::weighted_average<RGBQUAD>, // default RGB transformer
        unsigned plength /* palette length */ =
            palettes::palette_extended.size()> // the palette used by the mapper will default to palette_extended
    // imposing class level type constraints won't play well with the user defined default ctor as the compiler cannot deduce the types used in the requires expressions
    // hence, moving them to the custom ctor.
    class rgbmapper final {
        private:
            transformer_type             _rgbtransformer;
            std::array<wchar_t, plength> _palette;
            unsigned                     _palette_len;

        public:
            using size_type      = transformer_type::value_type; // will be used to index into the palette buffer
            using value_type     = wchar_t;                      // return type of operator()
            using converter_type = transformer_type;             // RGB to BW converter type
            using pixel_type     = ;

            constexpr rgbmapper() noexcept :
                _rgbtransformer(converter_type {}), _palette(palettes::palette_extended), _palette_len(plength) { }

            constexpr rgbmapper(const std::array<wchar_t, plength>& palette, const converter_type& transformer) noexcept requires requires {
                transformer.operator()();
                // argument is of type const RGBQUAD& so will work :)
                // rgb transformer must have a valid operator() that takes a reference to RGBQUAD defined!
                transformer_type::value_type; // and a public type alias called value_type
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
            constexpr value_type operator()(const RGBQUAD& pixel) const noexcept {
                // _rgbtransformer(pixel) can range from 0 to 255
                auto _offset { _rgbtransformer(pixel) };
                // hence, _offset / static_cast<float>(UCHAR_MAX) can range from 0.0 to 1.0
                return _palette[_offset ? (_offset / static_cast<float>(UCHAR_MAX) * _palette_len) - 1 : 0];
            }
    };

    [[nodiscard("expensive")]] static inline std::wstring to_string(_In_ const bmp::bmp& image) {
        const size_t npixels = (size_t) image._infhead.biHeight * image->infhead.biWidth;
        const size_t nwchars = npixels + (2LLU * image->infhead.biHeight); // one additional L'\r', L'\n' at the end of each line

        std::wstring buffer {};
        buffer.resize(nwchars * sizeof(wchar_t));
        if (buffer.empty()) {
            fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
            return buffer;
        }

        // clang-format off
    /*
    The pixel data is organized in rows from bottom to top and, within each row, from left to right. Each row is called a "scan line".
    If the image height is given as a negative number, then the rows are ordered from top to bottom.
    In most contemporary .BMP images, the pixel ordering seems to be bottom up. i.e,

    (pixel at the top left corner of the image)
    this will be the last pixel --> 10 11 12 13 14 15 16 17 18 19
                                    .............................
                                    00 01 02 03 04 05 06 07 08 09 <-- this is the first pixel in the buffer
                                                                    (pixel at the bottom right corner of the image)
    */
        // clang-format on

        size_t caret = 0;
        // if pixels are ordered top down. i.e the first pixel in the buffer is the one at the top left corner of the image.
        if (image->infhead.biHeight < 0) {
            for (int64_t nrows = 0; nrows < image->infhead.biHeight; ++nrows) {
                for (int64_t ncols = 0; ncols < image->infhead.biWidth; ++ncols)
                    buffer[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        }

        // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
        else {
            // traverse up along the height
            for (int64_t nrows = image->infhead.biHeight - 1LL; nrows >= 0; --nrows) {
                // traverse left to right inside "scan lines"
                for (int64_t ncols = 0; ncols < image->infhead.biWidth; ++ncols)
                    buffer[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        }

        assert(caret == nwchars);
        return (buffer_t) { buffer, caret };
    }

    // Generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140
    // chars) The total downscaling is completely predicated only on the image width, and the proportionate scaling effects will
    // automatically apply to the image height.

    static inline std::wstring to_downscaled_string(_In_ const bmp::bmp& image) {
        // downscaling needs to be done in pixel blocks.
        // each block will be represented by a single wchar_t
        const size_t block_s   = std::ceill(image.infhead.biWidth / 140.0L);
        const size_t block_dim = std::powl(block_s, 2.0000L);

        // We'd have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
        // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs

        const size_t nwchars   = 142 /* 140 wchar_ts + CRLF */ * std::ceill(image->infhead.biHeight / (long double) block_s);
        std::wstring buffer {};
        buffer.resize(nwchars * sizeof(wchar_t));

        if (buffer.empty()) {
            fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
            return buffer;
        }

        long double avg_B = 0.0, avg_G = 0.0, avg_R = 0.0;
        size_t      caret = 0, offset = 0;

        if (image->infhead.biHeight < 0) {
            for (int64_t nrows = 0; nrows < image->infhead.biHeight; nrows += block_s) {    // start traversal at the bottom most scan line
                for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols += block_s) { // traverse left to right in scan lines

                    // deal with blocks
                    for (int64_t bh = nrows; bh < (nrows + block_s); ++bh) {
                        for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                            offset  = (bh * image->infhead.biWidth) + bw;
                            avg_B  += image->pixel_buffer[offset].rgbBlue;
                            avg_G  += image->pixel_buffer[offset].rgbGreen;
                            avg_R  += image->pixel_buffer[offset].rgbRed;
                        }
                    }

                    avg_B           /= block_dim;
                    avg_G           /= block_dim;
                    avg_R           /= block_dim;
                    // pixel->rgbBlue * 0.299L + pixel->rgbGreen * 0.587L + pixel->rgbRed * 0.114L
                    buffer[caret++]  = char_array[(size_t) (avg_B * 0.299L + avg_G * 0.587L + avg_R * 0.114L) % __crt_countof(char_array)];
                    avg_B = avg_G = avg_R = 0.0L;
                }

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        } else {
            for (int64_t nrows  = image->infhead.biHeight - 1LLU; nrows >= 0;
                 nrows         -= block_s) {                                                        // start traversal at the bottom most scan line
                for (int64_t ncols = 0; ncols < image->infhead.biWidth; ncols += block_s) { // traverse left to right in scan lines

                    // deal with blocks
                    for (int64_t bh = nrows; bh > (nrows - block_s); --bh) {
                        for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                            offset  = (bh * image->infhead.biWidth) + bw;
                            avg_B  += image->pixel_buffer[offset].rgbBlue;
                            avg_G  += image->pixel_buffer[offset].rgbGreen;
                            avg_R  += image->pixel_buffer[offset].rgbRed;
                        }
                    }

                    avg_B           /= block_dim;
                    avg_G           /= block_dim;
                    avg_R           /= block_dim;
                    buffer[caret++]  = char_array[(size_t) ((avg_B + avg_G + avg_R) / 3) % __crt_countof(char_array)];
                    avg_B = avg_G = avg_R = 0.0L;
                }

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        }
        // wprintf_s(L"caret %5zu, nwchars %5zu\n", caret, nwchars);
        // assert(caret == nwchars); not likely :(
        return (buffer_t) { buffer, caret };
    }

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
            constexpr random_access_iterator() noexcept : _resource(), _offset(), _length() { }

            constexpr random_access_iterator(pointer _ptr, size_type _size) noexcept : _resource(_ptr), _offset(), _length(_size) { }

            constexpr random_access_iterator(pointer _ptr, size_type _pos, size_type _size) noexcept :
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

            constexpr random_access_iterator& operator++() noexcept { }

            constexpr random_access_iterator operator++(int) noexcept { }

            constexpr random_access_iterator& operator--() noexcept { }

            constexpr random_access_iterator operator--(int) noexcept { }

            constexpr reference operator*() noexcept { return _resource[_offset]; }

            constexpr const_reference operator*() const noexcept { return _resource[_offset]; }
    };

} // namespace utilities
