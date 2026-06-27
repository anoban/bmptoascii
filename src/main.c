#ifndef __TEST__
    #include <_tostring.h>

int main(const int argc, char* argv[]) {
    #ifdef _DEBUG

    const wchar_t* const bitmaps[] = { L"./test/bobmarley.bmp", L"./test/football.bmp", L"./test/garfield.bmp", L"./test/gewn.bmp",
                                       L"./test/girl.bmp",      L"./test/jennifer.bmp", L"./test/messi.bmp",    L"./test/supergirl.bmp",
                                       L"./test/time.bmp",      L"./test/uefa2024.bmp", L"./test/vendetta.bmp", NULL };
    const wchar_t**      _ptr      = bitmaps;
    while (*_ptr) {
        bitmap_t image                     = bmpread(*_ptr);
        const wchar_t* const restrict wstr = to_string(&image);
        if (!wstr) {
            wprintf_s(L"Error :: failed processing image %s!\n", *_ptr);
            bmpclose(&image);
            continue; // move on to the next image
        }

        _putws(wstr);
        _putws(L"\n\n");
        free(wstr);
        bmpclose(&image);
        _ptr++;
    }

    #else // N_DEBUG

    if (argc == 1) {
        fputws(L"Error :: Inappropriate invocation! Programme expects at least one path to a bitmap image\n", stderr);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        bitmap_t image                     = bmpread(argv[i]);
        const wchar_t* const restrict wstr = to_string(&image);
        if (!wstr) {
            wprintf_s(L"Error :: failed processing image %s!\n", argv[i]);
            bmpclose(&image);
            continue; // move on to the next image
        }

        _putws(wstr);
        _putws(L"\n\n");
        free(wstr);
        bmpclose(&image);
    }

    #endif

    return EXIT_SUCCESS;
}

#endif
