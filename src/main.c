#ifndef __TEST_BMPT_ASCII__

    #include <tostring.h>

int wmain(_In_opt_ const int32_t argc, _In_opt_count_(argc) wchar_t* argv[]) {
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    bitmap_t vendetta                          = bitmap_read(L"./vendetta.bmp");
    const wchar_t* const restrict vendettawstr = to_string(&vendetta);
    _putws(vendettawstr);
    _putws(L"\n\n");

    bitmap_t child                          = bitmap_read(L"./child.bmp");
    const wchar_t* const restrict childwstr = to_string(&child);
    _putws(childwstr);
    _putws(L"\n\n");

    bitmap_t girl                          = bitmap_read(L"./girl.bmp");
    const wchar_t* const restrict girlwstr = to_string(&girl);
    _putws(girlwstr);
    _putws(L"\n\n");

    bitmap_t bobmarley                          = bitmap_read(L"./bobmarley.bmp");
    const wchar_t* const restrict bobmarleywstr = to_string(&bobmarley);
    _putws(bobmarleywstr);
    _putws(L"\n\n");

    free(vendettawstr);
    free(childwstr);
    free(girlwstr);
    free(bobmarleywstr);

    bitmap_close(&vendetta);
    bitmap_close(&child);
    bitmap_close(&girl);
    bitmap_close(&bobmarley);

    return EXIT_SUCCESS;
}

#endif // !__TEST_BMPT_ASCII__
