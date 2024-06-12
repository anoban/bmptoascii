#include <bitmap.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USE <tostring.h> AS THE MASTER INCLUDE IN C SOURCES SINCE IT TRANSITIVELY INCLUDES <bitmap.h> AND <utilities.h> //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CONSOLE_WIDTH              140LLU

// selected palette for RGB to wchar_t mapping
#define spalette                   palette

// transformer to be used for raw string mapping
#define map(_pixel)                arithmetic_mapper(_pixel, spalette, __crt_countof(spalette))

// transformer to be used with block based downscaled string mapping
#define blockmap(blue, green, red) weighted_blockmapper(blue, green, red, spalette, __crt_countof(spalette))

static inline wchar_t* __cdecl to_raw_string(_In_ const bitmap_t* const restrict image) {
    const int64_t npixels = (int64_t) image->_infoheader.biHeight * image->_infoheader.biWidth; // total pixels in the image
    const int64_t nwchars /* 1 wchar_t for each pixel + 2 additional wchar_ts for CRLF at the end of each scanline */ =
        npixels + 2LLU * image->_infoheader.biHeight;
    // space for two extra wchar_ts (L'\r', L'\n') to be appended to the end of each line

    wchar_t* const restrict buffer = malloc(nwchars * sizeof(wchar_t));
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

// generate the wchar_t buffer after downscaling the image such that the ascii representation will fit the terminal width (~142 chars),
// downscaling is completely predicated only on the image width, and the proportionate scaling factor will be used to scale down the image vertically too.
// downscaling needs to be done in square pixel blocks which will be represented by a single wchar_t
static inline wchar_t* __cdecl to_downscaled_string(_In_ const bitmap_t* const restrict image) {
    const int64_t block_d /* dimension of an individual square block */         = ceill(image->_infoheader.biWidth / 140.0L);
    // even if the image is 150 pixels wide, nblocks_w will be ceill(150 / 140) = 2
    // each pixel block will be 2 pixels wide and 2 pixels tall (this 1:1 dimension of the block helps us preserve the shape of the original image)
    const double  blocksize /* number of pixels in a block */                   = block_d * block_d; // since our blocks are square
    int64_t       pblocksize /* number of pixels in a given incomplete block */ = 0;
    // if the image is 3720 pixels wide, 3720 / 140 = 26.5714285, hence block dim ~ 27 x 27
    // we'll need 3720 / 27 = 137.7777 blocks
    // first 137 blocks will represent 27 x 27 pixel blocks, and the last one will represent only a fraction of that block size
    const int64_t nblocks_w                                                     = ceill(image->_infoheader.biWidth / (double) block_d);
    const int64_t nblocks_h                                                     = ceill(image->_infoheader.biHeight / (double) block_d);

    // we have to compute the average R, G & B values for all pixels inside each pixel blocks and use the average to represent
    // that block as a wchar_t. one wchar_t in our buffer will have to represent (block_w x block_h) number of RGBQUADs
    const int64_t nwchars          = nblocks_h * (nblocks_w + 2); // saving two wide chars for CRLF!

    wchar_t* const restrict buffer = malloc(nwchars * sizeof(wchar_t));
    if (!buffer) {
        fwprintf_s(stderr, L"Error in %s @ line %d: malloc failed!\n", __FUNCTIONW__, __LINE__);
        return NULL;
    }

    // NOLINTBEGIN(readability-isolate-declaration)
    double  blockavg_blue = 0.0, blockavg_green = 0.0, blockavg_red = 0.0; // per block averages of the rgbBlue, rgbGreen and rgbRed values
    int64_t caret = 0, offset = 0;
    int64_t col = 0, row = 0;
    const bool has_incomplete_blocks_w =
        (image->_infoheader.biWidth % CONSOLE_WIDTH); // true if the image width is not divisible by 140 without ramainders
    const bool has_incomplete_blocks_h = image->_infoheader.biHeight % block_d;
    int64_t    incomplete_block_offset = 0;           // if there are unprocessed pixels in the current block row
    // NOLINTEND(readability-isolate-declaration)

    pblocksize                         = // number of pixels in each block in the rightmost column of incomplete blocks.
        (image->_infoheader.biWidth -
         image->_infoheader.biWidth / block_d /* deliberate integer division to get only the count of complete blocks */ * block_d) *
        block_d;
    assert(pblocksize < blocksize);

    if (image->_infoheader.biHeight < 0) { // [[unlikely]] if the image pixels are ordered topdown
        // these two nested for loops will not process the incompletely filled blocks! if there was an incomplete block at the end,
        // we'll have to process it at the end
        for (; row < image->_infoheader.biHeight; row += block_d) {
            // for all the complete blocks in a row,
            for (; col < image->_infoheader.biWidth; col += block_d) {
                // the outer two nested for loops will work to delimit the block the two inner nested for loops will operate on

                for (int64_t r = row; r < row + block_d; ++r) {     // r delimits the start row of the block being defined
                    for (int64_t c = col; c < col + block_d; ++c) { // c delimits the start column of the block being defined
                        offset          = (r * image->_infoheader.biWidth) + c;
                        blockavg_blue  += image->_pixels[offset].rgbBlue;
                        blockavg_green += image->_pixels[offset].rgbGreen;
                        blockavg_red   += image->_pixels[offset].rgbRed;
                    }
                }

                blockavg_blue  /= blocksize;
                blockavg_green /= blocksize;
                blockavg_red   /= blocksize;
                assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);

                buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
            }

            blockavg_blue = blockavg_green = blockavg_red = 0.000; // reset the block averages

            if (has_incomplete_blocks_w) {                         // if there are partially filled blocks at the end of this row of blocks,
                for (int64_t r = row; r < row + block_d; ++r) {
                    for (col -= block_d; // shift the column delimiter backward by one block, to the end of the last complete block
                         col < image->_infoheader.biWidth;
                         ++col) {        // start from the end of the last complete block
                        offset          = (r * image->_infoheader.biWidth) + col;
                        blockavg_blue  += image->_pixels[offset].rgbBlue;
                        blockavg_green += image->_pixels[offset].rgbGreen;
                        blockavg_red   += image->_pixels[offset].rgbRed;
                        pblocksize++;
                    }
                }

                blockavg_blue  /= pblocksize;
                blockavg_green /= pblocksize;
                blockavg_red   /= pblocksize;

                assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);
                buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
                blockavg_blue = blockavg_green = blockavg_red = 0.000; // reset the block averages
            }

            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
            col             = 0;
        }

        // process the last incomplete row of pixel blocks here,
        for (; col < image->_infoheader.biWidth; col += block_d) {
            for (int64_t r = 0; r < row + block_d; ++r) {     // r delimits the start row of the block being defined
                for (int64_t c = 0; c < col + block_d; ++c) { // c delimits the start column of the block being defined
                    offset          = (r * image->_infoheader.biWidth) + c;
                    blockavg_blue  += image->_pixels[offset].rgbBlue;
                    blockavg_green += image->_pixels[offset].rgbGreen;
                    blockavg_red   += image->_pixels[offset].rgbRed;
                }
            }

            blockavg_blue  /= blocksize;
            blockavg_green /= blocksize;
            blockavg_red   /= blocksize;
            assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);
            buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
        }

        buffer[caret++] = L'\n';
        buffer[caret++] = L'\r';
        col             = 0;

    } else { // if the image pixels are ordered bottom up
        row = image->_infoheader.biHeight - 1;
        for (; row >= 0; row -= block_d) {
            // start traversal at the bottom most scan line
            for (; col < image->_infoheader.biWidth; col += block_d) { // traverse left to right in scan lines

                // deal with blocks
                for (int64_t bh = row; bh > (row - block_d); --bh) {
                    for (int64_t bw = col; bw < (col + block_d); ++bw) {
                        offset          = (bh * image->_infoheader.biWidth) + bw;
                        blockavg_blue  += image->_pixels[offset].rgbBlue;
                        blockavg_green += image->_pixels[offset].rgbGreen;
                        blockavg_red   += image->_pixels[offset].rgbRed;
                    }
                }

                blockavg_blue  /= blocksize;
                blockavg_green /= blocksize;
                blockavg_red   /= blocksize;
                assert(blockavg_blue <= 255.00 && blockavg_green <= 255.00 && blockavg_red <= 255.00);
                buffer[caret++] = blockmap(blockavg_blue, blockavg_green, blockavg_red);
                blockavg_blue = blockavg_green = blockavg_red = 0.000;
            }

            buffer[caret++] = L'\n';
            buffer[caret++] = L'\r';
        }
    }
    // wprintf_s(L"caret %5zu, nwchars %5zu\n", caret, nwchars);
    // assert(caret == nwchars); not likely :(
    return buffer;
}

// an image width predicated dispatcher for to_raw_string and to_downscaled_string
static inline wchar_t* __cdecl to_string(_In_ const bitmap_t* const restrict image) {
    if (image->_infoheader.biWidth <= CONSOLE_WIDTH) return to_raw_string(image);
    return to_downscaled_string(image);
}
