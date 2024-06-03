#include <cmath>
#include <cstdio>
#include <string>

#include <bmp.hpp>

namespace ascii {

    [[nodiscard("expensive")]] static inline std::wstring to_raw_string(_In_ const bmp::bmp& image) {
        const auto npixels { image.height() * image.width() }; // total pixels in the image
        const auto nwchars { npixels + 2LLU * image.height() };
        // space for two extra wchar_ts (L'\r', L'\n') to be appended to the end of each line

        std::wstring buffer {};
        buffer.resize(nwchars * sizeof(wchar_t));
        if (buffer.empty()) {
            ::fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
            return buffer;
        }

        // pixels are organized in rows from bottom to top and, within each row, from left to right, each row is called a "scan line".
        // if the image height is given as a negative number, then the rows are ordered from top to bottom (in most contemporary .BMP images, the pixel ordering seems to be bottom up)
        // (pixel at the top left corner of the image)
        //                                            10 11 12 13 14 15 16 17 18 19 <-- this will be the last pixel (pixel at the bottom right corner of the image)
        //                                            .............................
        // this is the first pixel in the buffer -->  00 01 02 03 04 05 06 07 08 09
        // (pixel at the top left corner of the image)

        size_t caret {};
        // if pixels are ordered top down, (i.e the first pixel in the buffer is the one at the top left corner of the image)
        if (image.height() < 0) {
            for (size_t nrows {}; nrows < image.height(); ++nrows) {
                for (size_t ncols {}; ncols < image.width(); ++ncols)
                    buffer[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        }

        // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
        else {
            // traverse up along the height
            for (size_t nrows = image->infhead.biHeight - 1LL; nrows >= 0; --nrows) {
                // traverse left to right inside "scan lines"
                for (size_t ncols = 0; ncols < image->infhead.biWidth; ++ncols)
                    buffer[caret++] = __ScaleRgbQuadWAVG(&image->pixel_buffer[nrows * image->infhead.biWidth + ncols]);

                buffer[caret++] = L'\n';
                buffer[caret++] = L'\r';
            }
        }

        assert(caret == nwchars);
        return (buffer_t) { buffer, caret };
    }

    // generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140
    // chars), downscaling is completely predicated only on the image width, and the proportionate scaling effects will automatically apply to the image height.
    static inline std::wstring to_downscaled_string(_In_ const bmp::bmp& image) {
        // downscaling needs to be done in pixel blocks which will be represented by a single wchar_t
        const size_t block_s   = std::ceill(image.infhead.biWidth / 140.0L);
        const size_t block_dim = std::powl(block_s, 2.0000L);

        // we'd have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
        // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs
        const size_t nwchars   = 142 /* 140 wchar_ts + CRLF */ * std::ceill(image->infhead.biHeight / (long double) block_s);
        std::wstring buffer {};
        buffer.resize(nwchars * sizeof(wchar_t));

        if (buffer.empty()) {
            ::fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
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

} // namespace ascii
