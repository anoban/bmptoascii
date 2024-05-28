#include <ascii.hpp>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
    uint64_t fsize = 0;

    if (argc < 2) {
        ::fputws(L"Not enough arguments: main.exe expects one or more paths to .BMP files", stderr);
        return EXIT_FAILURE;
    }

    for (size_t i = 1; i < argc; ++i) {
        const uint8_t* const buffer = OpenImage(argv[i], &fsize);

        if (buffer) { // NewBmpImage will also nullptr check the buffer
            const bmp image = NewBmpImage(buffer, fsize);

            if (image.pixel_buffer) {
                // BmpInfo(&image);

                const buffer_t txt = GenerateASCIIBuffer(&image);
                if (txt.buffer) {
                    _putws(argv[i]);
                    _putws(txt.buffer);
                    free(txt.buffer);
                }
                free(image.pixel_buffer);
            }
            free(buffer);
        } else {
            fwprintf_s(stderr, L"Skipping image %s\n", argv[i]);
            continue;
        }
    }

    return EXIT_SUCCESS;
}
