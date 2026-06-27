#pragma once

// a proxy header to provide definitions for some essential structs

// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-rgbquad
typedef struct {
        unsigned char rgbBlue;
        unsigned char rgbGreen;
        unsigned char rgbRed;
        unsigned char rgbReserved;
} RGBQUAD;
static_assert(sizeof(RGBQUAD) == 4);

// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
typedef struct {
        unsigned       biSize;
        int            biWidth;
        int            biHeight;
        unsigned short biPlanes;
        unsigned short biBitCount;
        unsigned       biCompression;
        unsigned       biSizeImage;
        int            biXPelsPerMeter;
        int            biYPelsPerMeter;
        unsigned       biClrUsed;
        unsigned       biClrImportant;
} BITMAPINFOHEADER;
static_assert(sizeof(BITMAPINFOHEADER) == 40);

// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
#pragma pack(push, 2)
typedef struct {
        unsigned short bfType;
        unsigned       bfSize;
        unsigned short bfReserved1;
        unsigned short bfReserved2;
        unsigned       bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)
static_assert(sizeof(BITMAPFILEHEADER) == 14);
