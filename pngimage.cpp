#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib>
#include <cstdio>
#include <csetjmp>
#include <new>
using namespace std;

#include <png.h>
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng.lib")

#include "stream.h"

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

struct DataStreamIo
{
    DataStream& m_ds;
    INT m_i;
    INT m_cb;
    DataStreamIo(DataStream& ds, INT i, INT cb)
    : m_ds(ds), m_i(i), m_cb(cb)
    {
    }
};

static void PngReadProc(png_structp png, png_bytep data, png_size_t length)
{
    DataStreamIo *dsio = (DataStreamIo *)png_get_io_ptr(png);
    CopyMemory(data, dsio->m_ds.Ptr() + dsio->m_i, length);
    dsio->m_i += length;
}

static void PngWriteProc(png_structp png, png_bytep data, png_size_t length)
{
    DataStream *ds = (DataStream *)png_get_io_ptr(png);
    ds->Append(data, length);
}

HBITMAP LoadPngAsBitmapFromMemory(DataStream& ds, INT i, INT cb)
{
    HBITMAP         hbm;
    png_structp     png;
    png_infop       info;
    png_uint_32     y, width, height, rowbytes;
    int             color_type, depth, widthbytes;
    double          gamma;
    BITMAPINFO      bi;
    LPBYTE          pbBits;
    DataStreamIo    dsio(ds, i, cb);

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
    {
        return NULL;
    }

    info = png_create_info_struct(png);
    if (info == NULL || setjmp(png->jmpbuf))
    {
        png_destroy_read_struct(&png, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    png_set_read_fn(png, &dsio, PngReadProc);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_bgr(png);
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);
    else
        png_set_gamma(png, 2.2, 0.45455);

    png_read_update_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);

#ifdef PNG_FREE_ME_SUPPORTED
    png_free_data(png, info, PNG_FREE_ROWS, 0);
#endif

    rowbytes = png_get_rowbytes(png, info);
    if (info->row_pointers == NULL)
    {
        info->row_pointers = (png_bytepp)png_malloc(png,
            info->height * png_sizeof(png_bytep));
#ifdef PNG_FREE_ME_SUPPORTED
        info->free_me |= PNG_FREE_ROWS;
#endif
        for (y = 0; y < info->height; y++)
        {
            info->row_pointers[y] = (png_bytep)png_malloc(png, rowbytes);
        }
    }
    png_read_image(png, info->row_pointers);
    info->valid |= PNG_INFO_IDAT;
    png_read_end(png, NULL);

    ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = height;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = depth * png_get_channels(png, info);

    hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (VOID **)&pbBits,
                           NULL, 0);
    if (hbm == NULL)
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    widthbytes = WIDTHBYTES(width * bi.bmiHeader.biBitCount);
    for(y = 0; y < height; y++)
    {
        CopyMemory(pbBits + y * widthbytes,
                   info->row_pointers[height - 1 - y], rowbytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
    return hbm;
}

BOOL SaveBitmapAsPngFileToMemory(DataStream& ds, HBITMAP hbm)
{
    png_structp png;
    png_infop info;
    png_color_8 sBIT;
    png_bytep *lines;
    HDC hMemDC;
    HWND hWnd;
    HGDIOBJ hbmOld;
    RECT rc;
    SIZE siz;
    DWORD dwError;
    BITMAPINFO bi;
    BITMAP bm;
    DWORD dwWidthBytes, cbBits;
    LPBYTE pbBits;
    BOOL f;
    INT y;
    INT nDepth;

    if (GetObject(hbm, sizeof(BITMAP), &bm) != sizeof(BITMAP))
        return FALSE;

    nDepth = (bm.bmBitsPixel == 32 ? 32 : 24);
    dwWidthBytes = WIDTHBYTES(bm.bmWidth * nDepth);
    cbBits = dwWidthBytes * bm.bmHeight;
    pbBits = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, cbBits);
    if (pbBits == NULL)
        return FALSE;

    f = FALSE;
    hMemDC = CreateCompatibleDC(NULL);
    if (hMemDC != NULL)
    {
        ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
        bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth    = bm.bmWidth;
        bi.bmiHeader.biHeight   = bm.bmHeight;
        bi.bmiHeader.biPlanes   = 1;
        bi.bmiHeader.biBitCount = nDepth;
        f = GetDIBits(hMemDC, hbm, 0, bm.bmHeight, pbBits, &bi,
                      DIB_RGB_COLORS);
        DeleteDC(hMemDC);
    }
    if (!f)
    {
        HeapFree(GetProcessHeap(), 0, pbBits);
        return FALSE;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pbBits);
        return FALSE;
    }

    info = png_create_info_struct(png);
    if (info == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pbBits);
        png_destroy_write_struct(&png, NULL);
        return FALSE;
    }

    lines = NULL;
    if (setjmp(png_jmpbuf(png)))
    {
        HeapFree(GetProcessHeap(), 0, pbBits);
        DeleteObject(hbm);
        if (lines != NULL)
            HeapFree(GetProcessHeap(), 0, lines);
        return FALSE;
    }

    png_set_IHDR(png, info, bm.bmWidth, bm.bmHeight, 8,
        (nDepth == 32 ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);

    sBIT.red = 8;
    sBIT.green = 8;
    sBIT.blue = 8;
    sBIT.alpha = (nDepth == 32 ? 8 : 0);
    png_set_sBIT(png, info, &sBIT);

    png_set_write_fn(png, &ds, PngWriteProc, NULL);

    png_write_info(png, info);
    png_set_bgr(png);

    lines = (png_bytep *)HeapAlloc(GetProcessHeap(), 0,
                                   sizeof(png_bytep *) * bm.bmHeight);
    for (y = 0; y < bm.bmHeight; y++)
        lines[y] = (png_bytep)&pbBits[dwWidthBytes * (bm.bmHeight - y - 1)];

    png_write_image(png, lines);
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    HeapFree(GetProcessHeap(), 0, pbBits);
    HeapFree(GetProcessHeap(), 0, lines);
    DeleteObject(hbm);

    return TRUE;
}

static void PngReadProc2(png_structp png, png_bytep data, png_size_t length)
{
    ReadFile(png_get_io_ptr(png), data, length, (DWORD*)&length, NULL);
}

HBITMAP LoadPngAsBitmap(LPCSTR pszFileName)
{
    HANDLE          hFile;
    HBITMAP         hbm;
    png_structp     png;
    png_infop       info;
    png_uint_32     y, width, height, rowbytes;
    int             color_type, depth, widthbytes;
    double          gamma;
    BITMAPINFO      bi;
    LPBYTE          pbBits;
    
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
    {
        CloseHandle(hFile);
        return NULL;
    }

    info = png_create_info_struct(png);
    if (info == NULL || setjmp(png->jmpbuf))
    {
        png_destroy_read_struct(&png, NULL, NULL);
        CloseHandle(hFile);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        CloseHandle(hFile);
        return NULL;
    }

    png_set_read_fn(png, hFile, PngReadProc2);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_bgr(png);
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);
    else
        png_set_gamma(png, 2.2, 0.45455);

    png_read_update_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);

#ifdef PNG_FREE_ME_SUPPORTED
    png_free_data(png, info, PNG_FREE_ROWS, 0);
#endif

    rowbytes = png_get_rowbytes(png, info);
    if (info->row_pointers == NULL)
    {
        info->row_pointers = (png_bytepp)png_malloc(png,
            info->height * png_sizeof(png_bytep));
#ifdef PNG_FREE_ME_SUPPORTED
        info->free_me |= PNG_FREE_ROWS;
#endif
        for (y = 0; y < info->height; y++)
        {
            info->row_pointers[y] = (png_bytep)png_malloc(png, rowbytes);
        }
    }
    png_read_image(png, info->row_pointers);
    info->valid |= PNG_INFO_IDAT;
    png_read_end(png, NULL);
    CloseHandle(hFile);

    ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = height;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = depth * png_get_channels(png, info);

    hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (VOID **)&pbBits, 
                           NULL, 0);
    if (hbm == NULL)
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)
    widthbytes = WIDTHBYTES(width * bi.bmiHeader.biBitCount);
    for(y = 0; y < height; y++)
    {
        CopyMemory(pbBits + y * widthbytes, 
                   info->row_pointers[height - 1 - y], rowbytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
    return hbm;
}
