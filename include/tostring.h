#pragma once

#include <bitmap.h>

#define CONSOLE_WIDTH  140LL
#define CONSOLE_WIDTHR 140.0

////////////////////////////////////
//    PLACE FOR CUSTOMIZATIONS    //
////////////////////////////////////
#define spalette       palette_extended                                           // PICK ONE OF THE THREE AVALIABLE PALETTES
#define map(_pixel)    weighted_mapper(_pixel, spalette, __crt_countof(spalette)) // CHOOSE A BASIC MAPPER OF YOUR LIKING
#define blockmap(blue, green, red)                                                                                                         \
    weighted_blockmapper(blue, green, red, spalette, __crt_countof(spalette)) // CHOOSE A BLOCK MAPPER OF YOUR LIKING
// IT IS NOT OBLIGATORY FOR BOTH THE BASIC MAPPER AND THE BLOCK MAPPER TO USE THE SAME PALETTE
// IF NEED BE, THE PALETTE EXPANDED FROM spalette COULD BE REPLACED BY A REAL PALETTE NAME

static inline wchar_t* __cdecl to_raw_string(_In_ const bitmap_t* const restrict image) {
    if (image->_infoheader.biHeight < 0) {
        fputws(L"Error in to_raw_string, this tool does not support bitmaps with top-down pixel ordering!\n", stderr);
        return NULL;
    }

    const int64_t npixels = (int64_t) image->_infoheader.biHeight * image->_infoheader.biWidth; // total pixels in the image
    const int64_t nwchars /* 1 wchar_t for each pixel + 2 additional wchar_ts for CRLF at the end of each scanline */ =
        npixels + 2LLU * image->_infoheader.biHeight;
    // space for two extra wchar_ts (L'\r', L'\n') to be appended to the end of each line

    wchar_t* const restrict buffer = malloc((nwchars + 1) * sizeof(wchar_t)); // and the +1 is for the NULL terminator
    if (!buffer) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return NULL;
    }

    // pixels are organized in rows from bottom to top and, within each row, from left to right, each row is called a "scan line".
    // if the image height is given as a negative number, then the rows are ordered from top to bottom (in most contemporary .BMP images, the pixel ordering seems to be bottom up)
    // (pixel at the top left corner of the image)
    //                                            10 11 12 13 14 15 16 17 18 19 <-- this will be the last pixel (pixel at the bottom right corner of the image)
    //                                            .............................
    // this is the first pixel in the buffer -->  00 01 02 03 04 05 06 07 08 09
    // (pixel at the top left corner of the image)

    int64_t caret = 0;
    // presuming pixels are ordered bottom up, start the traversal from the last pixel and move up.
    // traverse up along the height, for each row, starting with the last row,
    for (int64_t nrows = image->_infoheader.biHeight - 1LL; nrows >= 0; --nrows) {
        // traverse left to right inside "scan lines"
        for (int64_t ncols = 0; ncols < image->_infoheader.biWidth; ++ncols) // for each pixel in the row,
            buffer[caret++] = map(&image->_pixels[nrows * image->_infoheader.biWidth + ncols]);
        // at the end of each scanline, append a CRLF!
        buffer[caret++] = L'\n';
        buffer[caret++] = L'\r';
    }

    buffer[caret] = 0; // null termination of the string

    assert(caret == nwchars);
    return buffer;
}

// generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width (~142 chars),
// downscaling is completely predicated only on the image width, and the proportionate scaling factor will be used to scale down the image vertically too.
// downscaling needs to be done in square pixel blocks which will be represented by a single wchar_t
static inline wchar_t* __cdecl to_downscaled_string(_In_ const bitmap_t* const restrict image) {
    if (image->_infoheader.biHeight < 0) {
        fputws(L"Error in to_raw_string, this tool does not support bitmaps with top-down pixel ordering!\n", stderr);
        return NULL;
    }

    const int64_t block_d /* dimension of an individual square block */ = ceill(image->_infoheader.biWidth / CONSOLE_WIDTHR);

    const float blocksize /* number of pixels in a block */             = block_d * block_d; // since our blocks are square
    int64_t     pblocksize = // number of pixels in each block in the rightmost column of incomplete blocks.
        // width of the image - (number of complete blocks * block dimension) will give the residual pixels along the horizontal axis
        // multiply that by block domension again, and we'll get the number of pixels in the incomplete block
        (image->_infoheader.biWidth -
         (image->_infoheader.biWidth / block_d) /* deliberate integer division to get only the count of complete blocks */ * block_d) *
        block_d;
    assert(pblocksize < blocksize);

    const int64_t nblocks_w        = ceill(image->_infoheader.biWidth / (float) block_d);
    const int64_t nblocks_h        = ceill(image->_infoheader.biHeight / (float) block_d);

    // we have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
    // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs
    const int64_t nwchars          = nblocks_h * (nblocks_w + 2); // saving two wide chars for CRLF!

    wchar_t* const restrict buffer = malloc(nwchars * sizeof(wchar_t));
    if (!buffer) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return NULL;
    }

    // NOLINTBEGIN(readability-isolate-declaration)
    float   blockavg_blue = 0.0, blockavg_green = 0.0, blockavg_red = 0.0; // per block averages of the rgbBlue, rgbGreen and rgbRed values
    int64_t caret = 0, offset = 0, col = 0, row = 0;
    const bool block_rows_end_with_incomplete_blocks    = (image->_infoheader.biWidth % CONSOLE_WIDTH);
    // true if the image width is not divisible by 140 without remainders
    const bool block_columns_end_with_incomplete_blocks = image->_infoheader.biHeight % block_d;
    // true if the image height is not divisible by block_d without remainders
    // NOLINTEND(readability-isolate-declaration)

    uint64_t full = 0, incomplete = 0, count = 0;

    // row = image->_infoheader.biHeight will get us to the last pixel of the first (last in the buffer) scanline with (r * image->_infoheader.biWidth)
    // hence, row = image->_infoheader.biHeight - 1 so we can traverse the first scanline with (r * image->_infoheader.biWidth) + c
    for (row = image->_infoheader.biHeight - 1; row >= block_d; row -= block_d) { // start the traversal at the bottom most scan line
        wprintf_s(L"row = %lld\n", row);
        for (col = 0; col <= image->_infoheader.biWidth - block_d; col += block_d) { // traverse left to right in scan lines

            // wprintf_s(L"row = %lld, col = %lld\n", row, col);

            for (int64_t r = row; r > row - block_d; --r) { // deal with blocks
                for (int64_t c = col; c < col + block_d; ++c) {
                    offset          = (r * image->_infoheader.biWidth) + c;
                    blockavg_blue  += image->_pixels[offset].rgbBlue;
                    blockavg_green += image->_pixels[offset].rgbGreen;
                    blockavg_red   += image->_pixels[offset].rgbRed;

                    count++;
                }
            }

            full++;
            assert(count == block_d * block_d);
            count           = 0;

            blockavg_blue  /= blocksize;
            blockavg_green /= blocksize;
            blockavg_red   /= blocksize;

            assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);

            buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
            blockavg_blue = blockavg_green = blockavg_red = 0.000;
        }

        wprintf_s(L"col = %lld\n", col);

        if (block_rows_end_with_incomplete_blocks) { // if there are partially filled blocks at the end of this row of blocks,

            for (int64_t r = row; r > row - block_d; --r) {
                // shift the column delimiter backward by one block, to the end of the last complete block
                for (int64_t c = col; c < image->_infoheader.biWidth; ++c) { // start from the end of the last complete block
                    offset          = (r * image->_infoheader.biWidth) + c;
                    blockavg_blue  += image->_pixels[offset].rgbBlue;
                    blockavg_green += image->_pixels[offset].rgbGreen;
                    blockavg_red   += image->_pixels[offset].rgbRed;

                    count++;
                }
            }

            incomplete++;
            assert(count == pblocksize); // fails
            count           = 0;

            blockavg_blue  /= pblocksize;
            blockavg_green /= pblocksize;
            blockavg_red   /= pblocksize;

            assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);

            buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
            blockavg_blue = blockavg_green = blockavg_red = 0.000; // reset the block averages
        }

        buffer[caret++] = L'\n';
        buffer[caret++] = L'\r';
    }

    wprintf_s(L"full = %llu\n", full);
    wprintf_s(L"incomplete = %llu\n", incomplete);
    wprintf_s(L"row = %lld\n", row);
    assert(row < block_d);

    pblocksize = // change the block size to represent the number of pixels held by the last row blocks
        (image->_infoheader.biHeight - (image->_infoheader.biHeight / block_d /* deliberate integer division */) * block_d) * block_d;

    if (block_columns_end_with_incomplete_blocks) { // process the last incomplete row of pixel blocks here,

        for (col = 0; col < image->_infoheader.biWidth; col += block_d) { // col must be 0 at the start of this loop
            incomplete = 0;
            for (int64_t r = row; r >= 0; --r) {                // r delimits the start row of the block being defined
                for (int64_t c = col; c < col + block_d; ++c) { // c delimits the start column of the block being defined
                    offset          = (r * image->_infoheader.biWidth) + c;
                    blockavg_blue  += image->_pixels[offset].rgbBlue;
                    blockavg_green += image->_pixels[offset].rgbGreen;
                    blockavg_red   += image->_pixels[offset].rgbRed;
                }
            }

            blockavg_blue  /= pblocksize;
            blockavg_green /= pblocksize;
            blockavg_red   /= pblocksize;

            assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);
            buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
        }

        buffer[caret++] = L'\n';
        buffer[caret++] = L'\r';
    }

    //  buffer[caret] = 0; // using the last byte as null terminator
    wprintf_s(L"caret :: %lld, nwchars :: %lld\n", caret, nwchars);
    assert(caret == nwchars);
    return buffer;
}

// an image width predicated dispatcher for to_raw_string and to_downscaled_string
static inline wchar_t* __cdecl to_string(_In_ const bitmap_t* const restrict image) {
    if (image->_infoheader.biWidth <= CONSOLE_WIDTH) return to_raw_string(image);
    return to_downscaled_string(image);
}
