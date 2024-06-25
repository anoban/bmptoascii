#ifndef __TEST_BMPTOASCII__
    #include <tostring.h>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
    #ifdef _DEBUG

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    const wchar_t* const bitmaps[] = { L"child.bmp", L"football.bmp", L"jennifer.bmp", L"messi.bmp", L"supergirl.bmp", L"sydney.bmp",
                                       L"time.bmp",  L"vendetta.bmp", L"woman.bmp",    L"face.bmp",  L"lady.bmp",      NULL };
    const wchar_t**      _ptr      = bitmaps;
    while (*_ptr) {
        bitmap_t image                     = bitmap_read(*_ptr);
        const wchar_t* const restrict wstr = to_string(&image);
        if (!wstr) {
            wprintf_s(L"Error :: failed processing image %s!\n", *_ptr);
            bitmap_close(&image);
            continue; // move on to the next image
        }

        _putws(wstr);
        _putws(L"\n\n");
        free(wstr);
        bitmap_close(&image);
        _ptr++;
    }

    #else // N_DEBUG

    if (argc == 1) {
        fputws(L"Error :: Inappropriate invocation! Programme expects at least one path to a bitmap image\n", stderr);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        bitmap_t image                     = bitmap_read(argv[i]);
        const wchar_t* const restrict wstr = to_string(&image);
        if (!wstr) {
            wprintf_s(L"Error :: failed processing image %s!\n", argv[i]);
            bitmap_close(&image);
            continue; // move on to the next image
        }

        _putws(wstr);
        _putws(L"\n\n");
        free(wstr);
        bitmap_close(&image);
    }

    #endif

    return EXIT_SUCCESS;
}

#endif // !__TEST_BMPTOASCII__
