#ifndef __TEST_BMPT_ASCII__

    #include <tostring.h>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    const bitmap_t image                = bitmap_read(L"./vendetta.bmp");
    const wchar_t* const restrict wstrg = to_downscaled_string(&image);
    _putws(wstrg);
    free(wstrg);
    free(image._buffer);

    return EXIT_SUCCESS;
}

#endif // !__TEST_BMPT_ASCII__