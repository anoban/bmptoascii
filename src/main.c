#ifndef __TEST_BMPTOASCII__

    #include <tostring.h>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
    #ifdef _DEBUG

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    bitmap_t image                          = bitmap_read(L"./football.bmp");
    const wchar_t* const restrict bitmapstr = to_string(&image);
    _putws(bitmapstr);
    _putws(L"\n");

    free(bitmapstr);
    bitmap_close(&image);

    #else

    if (argc == 1) {
        _putws(L"");
        return EXIT_FAILURE;
    }

    for (unsigned i = 1; i < argc; ++i) {
        bitmap_t image                     = bitmap_read(argv[i]);
        const wchar_t* const restrict wstr = to_string(&image);
        _putws(wstr);
        _putws(L"\n\n");
        free(wstr);
        bitmap_close(&image);
    }

    #endif

    return EXIT_SUCCESS;
}

#endif // !__TEST_BMPTOASCII__
