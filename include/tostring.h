#include <bitmap.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USE <tostring.h> AS THE MASTER INCLUDE IN C SOURCES SINCE IT INCLUDES <bitmap.h>, WHICH INCLUDES <utilities.h> //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// selected palette for RGB to wchar_t mapping
#define spalette                   palette_extended

// transformer to be used for raw string mapping
#define map(_pixel)                arithmetic_mapper(_pixel, spalette, __crt_countof(spalette))

// transformer to be used with block based downscaled string mapping
#define blockmap(blue, green, red) minmax_blockmapper(blue, green, red, spalette, __crt_countof(spalette))

static inline wchar_t* to_raw_string(_In_ const bitmap_t* const restrict image) {
    const int64_t npixels          = (int64_t) image->_infoheader.biHeight * image->_infoheader.biWidth; // total pixels in the image
    const int64_t nwchars          = npixels + 2LLU * image->_infoheader.biHeight;
    // space for two extra wchar_ts (L'\r', L'\n') to be appended to the end of each line

    wchar_t* const restrict buffer = malloc(nwchars * sizeof(wchar_t));
    if (!buffer) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return buffer;
    }

    // pixels are organized in rows from bottom to top and, within each row, from left to right, each row is called a "scan line".
    // if the image height is given as a negative number, then the rows are ordered from top to bottom (in most contemporary .BMP images, the pixel ordering seems to be bottom up)
    // (pixel at the top left corner of the image)
    //                                            10 11 12 13 14 15 16 17 18 19 <-- this will be the last pixel (pixel at the bottom right corner of the image)
    //                                            .............................
    // this is the first pixel in the buffer -->  00 01 02 03 04 05 06 07 08 09
    // (pixel at the top left corner of the image)

    size_t caret = 0;
    // if pixels are ordered top down, (i.e the first pixel in the buffer is the one at the top left corner of the image)
    if (image->_infoheader.biHeight < 0) {
        // for each row, starting with the first row,
        for (int64_t nrows = 0; nrows < image->_infoheader.biHeight; ++nrows) {
            for (int64_t ncols = 0; ncols < image->_infoheader.biWidth; ++ncols) // for each pixel in the row,
                buffer[caret++] = map(&image->_pixels[nrows * image->_infoheader.biWidth + ncols]);
            // at the end of each scanline, append a CRLF!
            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
        }
    } else { // if pixels are ordered bottom up, start the traversal from the last pixel and move up.
        // traverse up along the height, for each row, starting with the last row,
        for (int64_t nrows = image->_infoheader.biHeight - 1LL; nrows >= 0; --nrows) {
            // traverse left to right inside "scan lines"
            for (int64_t ncols = 0; ncols < image->_infoheader.biWidth; ++ncols) // for each pixel in the row,
                buffer[caret++] = map(&image->_pixels[nrows * image->_infoheader.biWidth + ncols]);
            // at the end of each scanline, append a CRLF!
            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
        }
    }

    assert(caret == nwchars);
    return buffer;
}

// generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width. (140
// chars), downscaling is completely predicated only on the image width, and the proportionate scaling effects will automatically apply to the image height.
static inline wchar_t* to_downscaled_string(_In_ const bitmap_t* const restrict image) {
    // downscaling needs to be done in pixel blocks which will be represented by a single wchar_t
    const int64_t block_s          = ceill(image->_infoheader.biWidth / 140.0L);
    const int64_t block_dim        = powl(block_s, 2.0000L);

    // we'd have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
    // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs
    const int64_t nwchars          = 142 /* 140 wchar_ts + CRLF */ * ceill(image->_infoheader.biHeight / (long double) block_s);
    wchar_t* const restrict buffer = malloc(nwchars * sizeof(wchar_t));

    if (!buffer) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return buffer;
    }

    double  avg_B = 0.0, avg_G = 0.0, avg_R = 0.0;
    int64_t caret = 0, offset = 0;

    if (image->_infoheader.biHeight < 0) {
        for (int64_t nrows = 0; nrows < image->_infoheader.biHeight; nrows += block_s) {    // start traversal at the bottom most scan line
            for (int64_t ncols = 0; ncols < image->_infoheader.biWidth; ncols += block_s) { // traverse left to right in scan lines

                // deal with blocks
                for (int64_t bh = nrows; bh < (nrows + block_s); ++bh) {
                    for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                        offset  = (bh * image->_infoheader.biWidth) + bw;
                        avg_B  += image->_pixels[offset].rgbBlue;
                        avg_G  += image->_pixels[offset].rgbGreen;
                        avg_R  += image->_pixels[offset].rgbRed;
                    }
                }

                avg_B           /= block_dim;
                avg_G           /= block_dim;
                avg_R           /= block_dim;

                buffer[caret++]  = blockmap(avg_B, avg_G, avg_R);
                avg_B = avg_G = avg_R = 0.000; // reset the block averages
            }

            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
        }
    } else {
        for (int64_t nrows = image->_infoheader.biHeight - 1LLU; nrows >= 0; nrows -= block_s) {
            // start traversal at the bottom most scan line
            for (int64_t ncols = 0; ncols < image->_infoheader.biWidth; ncols += block_s) { // traverse left to right in scan lines

                // deal with blocks
                for (int64_t bh = nrows; bh > (nrows - block_s); --bh) {
                    for (int64_t bw = ncols; bw < (ncols + block_s); ++bw) {
                        offset  = (bh * image->_infoheader.biWidth) + bw;
                        avg_B  += image->_pixels[offset].rgbBlue;
                        avg_G  += image->_pixels[offset].rgbGreen;
                        avg_R  += image->_pixels[offset].rgbRed;
                    }
                }

                avg_B           /= block_dim;
                avg_G           /= block_dim;
                avg_R           /= block_dim;
                buffer[caret++]  = blockmap(avg_B, avg_G, avg_R);
                avg_B = avg_G = avg_R = 0.000;
            }

            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
        }
    }
    // wprintf_s(L"caret %5zu, nwchars %5zu\n", caret, nwchars);
    // assert(caret == nwchars); not likely :(
    return buffer;
}
