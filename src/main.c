#include <ascii.h>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
#ifndef _DEBUG

    if (argc < 2) {
        fwprintf_s(stderr, L"Not enough arguments: main.exe expects one or more paths to .BMP files\n");
        return EXIT_FAILURE;
    }

    uint64_t fsize = 0;

    for (size_t i = 1; i < argc; ++i) {
        const uint8_t* buffer = OpenImage(argv[i], &fsize);

        if (buffer) {
            const WinBMP image = NewBmpImage(buffer, fsize);

            if (image.pixel_buffer) {
                BmpInfo(&image);

                const ascii_t txt = GenerateASCIIBuffer(&image);
                if (txt.buffer) {
                    for (size_t i = 0; i < txt.length; ++i) {
                        // putwchar(txt.buffer[i]);
                        // printf_s("%d: %c ", txt.buffer[i], txt.buffer[i]);
                        putwchar(txt.buffer[i]);
                    }
                    free(txt.buffer);
                }
                free(image.pixel_buffer);
            }

        } else {
            fwprintf_s(stderr, L"Skipping image %s\n", argv[i]);
            continue;
        }
    }

#else

#endif // !_DEBUG

    uint64_t       fsize  = 0;

    const uint8_t* buffer = OpenImage(L"./media/flower.bmp", &fsize);
    const WinBMP   image  = NewBmpImage(buffer, fsize);
    BmpInfo(&image);

    const buffer_t txt = GenerateASCIIBuffer(&image);
    _putws(txt.buffer);

    free(txt.buffer);
    free(image.pixel_buffer);

    return EXIT_SUCCESS;
}