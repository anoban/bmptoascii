#include <utilities.hpp>

namespace utilities {

    namespace transformers {

        // a functor giving back the arithmetic average of an RGB pixel values
        template<typename pixel_type, typename return_type, typename> requires is_rgb<pixel_type>
        struct arithmetic_average { // struct arithmetic_average also doubles as the base class for other functors
                // deriving from arithmetic_average purely to leverage the requires is_rgb<pixel_type> constraint through inheritance with minimal redundancy
                using value_type = return_type;
                [[nodiscard]] constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(
                        (/* we don't want overflows or truncations here */ static_cast<double>(pixel.rgbBlue) + pixel.rgbGreen +
                         pixel.rgbRed) /
                        3.000L
                    );
                }
        };

        template<typename pixel_type, typename return_type>
        struct weighted_average final : public arithmetic_average<pixel_type, return_type> {
                // using arithmetic_average<pixel_type, return_type>::value_type makes the signature extremely verbose
                using value_type = return_type;
                // weighted average of an RGB pixel values
                [[nodiscard]] constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(pixel.rgbBlue * 0.299L + pixel.rgbGreen * 0.587L + pixel.rgbRed * 0.114L);
                }
        };

        template<typename pixel_type, typename return_type>
        struct minmax_average final : public arithmetic_average<pixel_type, return_type> {
                using value_type = return_type;
                // average of minimum and maximum RGB values in a pixel
                [[nodiscard]] constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(
                        (/* we don't want overflows or truncations here */ static_cast<double>(
                             std::min({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })
                         ) +
                         std::max({ pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed })) /
                        2.0000L
                    );
                }
        };

        template<typename pixel_type, typename return_type> struct luminosity final : public arithmetic_average<pixel_type, return_type> {
                using value_type = return_type;
                // luminosity of an RGB pixel
                [[nodiscard]] constexpr value_type operator()(const pixel_type& pixel) const noexcept {
                    return static_cast<return_type>(pixel.rgbBlue * 0.2126L + pixel.rgbGreen * 0.7152L + pixel.rgbRed * 0.0722L);
                }
        };

    } // namespace transformers

    // a composer that uses a specified pair of palette and RGB to BW mapper to return an appropriate wchar_t
    template<typename transformer_type, unsigned plength> class rgbmapper final {
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
            [[nodiscard]] constexpr value_type operator()(const RGBQUAD& pixel) const noexcept {
                // _rgbtransformer(pixel) can range from 0 to 255
                auto _offset { _rgbtransformer(pixel) };
                // hence, _offset / static_cast<float>(UCHAR_MAX) can range from 0.0 to 1.0
                return _palette[_offset ? (_offset / static_cast<float>(UCHAR_MAX) * _palette_len) - 1 : 0];
            }
    };

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
