#include <utilities.hpp>

template<typename T, typename = std::enable_if<std::is_arithmetic_v<T>, bool>::type> struct RGBPIXEL {
        T rgbRed {};
        T rgbGreen {};
        T rgbBlue {};
};

struct pix {
        unsigned char x {}, y {}, z {};
};

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) { // NOLINT(modernize-avoid-c-arrays)
    //
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    constexpr RGBQUAD maxrgb { 0xFF, 0xFF, 0xFF, 0xFF };
    constexpr RGBQUAD minrgb { 0x00, 0x00, 0x00, 0xFF };

    constexpr auto wavmapper { utilities::rgbmapper {} };
    constexpr auto arithmapper { utilities::rgbmapper<utilities::transformers::arithmetic_average<>> {} };

    constexpr auto max = utilities::transformers::weighted_average<RGBQUAD> {}(maxrgb);
    constexpr auto min = utilities::transformers::weighted_average<RGBQUAD> {}(minrgb);

    constexpr auto trans { utilities::transformers::luminosity<RGBPIXEL<long>> {} };
    constexpr auto transf { utilities::transformers::luminosity<RGBPIXEL<double>> {} };
    constexpr auto transfo { utilities::transformers::luminosity<RGBPIXEL<unsigned char>> {} };

    constexpr auto transfor { utilities::transformers::luminosity<pix> {} };

    constexpr auto x { wavmapper.operator()(minrgb) };
    constexpr auto y { arithmapper(maxrgb) };

    return EXIT_SUCCESS;
}
