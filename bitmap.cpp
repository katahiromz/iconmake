#include <windows.h>

#include <new>
using namespace std;

#include "stream.h"
#include "IconMake.h"
#include "pngimage.h"
#include "resource.h"

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)
#define GetAValue(clr) ((BYTE)((clr)>>24))

typedef struct tagLOGPALETTEEX
{
    WORD         palVersion; 
    WORD         palNumEntries; 
    PALETTEENTRY palPalEntry[255]; 
} LOGPALETTEEX;

HPALETTE CreatePaletteDx(INT nNumColors, RGBQUAD *prgbColors)
{
    INT i;
    LOGPALETTEEX pal;
    pal.palVersion = 0x300;
    pal.palNumEntries = nNumColors;
    for(i = 0; i < nNumColors; i++)
    {
        pal.palPalEntry[i].peRed        = prgbColors[i].rgbRed;
        pal.palPalEntry[i].peGreen      = prgbColors[i].rgbGreen; 
        pal.palPalEntry[i].peBlue       = prgbColors[i].rgbBlue;
        pal.palPalEntry[i].peFlags      = 0;
    }
    return CreatePalette((LOGPALETTE *)&pal);
}

WORD DIBNumColors(LPVOID pbi)
{
    WORD wBitCount;
    DWORD dwClrUsed;

    dwClrUsed = ((PBITMAPINFOHEADER)pbi)->biClrUsed;

    if (dwClrUsed)
        return (WORD) dwClrUsed;

    wBitCount = ((PBITMAPINFOHEADER)pbi)->biBitCount;

    switch (wBitCount)
    {
        case 1:     return 2;
        case 4:     return 16;
        case 8:     return 256;
        default:    return 0;
    }
}

DWORD BytesPerLine(PBITMAPINFOHEADER pbmih)
{
    return WIDTHBYTES(pbmih->biWidth * pbmih->biBitCount);
}

HBITMAP LoadBitmapFromFile(LPCTSTR pszFileName)
{
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    BITMAPINFOEX bi;
    DWORD cb, cbImage;
    DWORD dwError;
    LPVOID pBits, pBits2;
    HDC hDC, hMemDC;
    HBITMAP hbm;
    HGDIOBJ hbmOld;
    INT nNumColors;
    HPALETTE hPal, hPalOld;

    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    if (!ReadFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, NULL))
    {
        dwError = GetLastError();
        CloseHandle(NULL);
        SetLastError(dwError);
        return NULL;
    }

    pBits = NULL;
    if (bf.bfType == 0x4D42 && bf.bfReserved1 == 0 && bf.bfReserved2 == 0 &&
        bf.bfSize > bf.bfOffBits && bf.bfOffBits > sizeof(BITMAPFILEHEADER) &&
        bf.bfOffBits <= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOEX))
    {
        cbImage = bf.bfSize - bf.bfOffBits;
        pBits = HeapAlloc(GetProcessHeap(), 0, cbImage);
        if (pBits != NULL)
        {
            if (ReadFile(hFile, &bi, bf.bfOffBits -
                         sizeof(BITMAPFILEHEADER), &cb, NULL) &&
                ReadFile(hFile, pBits, cbImage, &cb, NULL))
            {
                ;
            }
            else
            {
                dwError = GetLastError();
                HeapFree(GetProcessHeap(), 0, pBits);
                pBits = NULL;
            }
        }
        else
            dwError = GetLastError();
    }
    else
        dwError = (DWORD)-(LONG)IDS_INVALID;

    CloseHandle(hFile);

    if (pBits == NULL)
    {
        SetLastError(dwError);
        return NULL;
    }

    hbm = NULL;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hMemDC = CreateCompatibleDC(hDC);
        if (hMemDC != NULL)
        {
            if (bi.bmiHeader.biBitCount >= 16)
            {
                hbm = CreateDIBSection(hMemDC, (BITMAPINFO*)&bi, 
                                       DIB_RGB_COLORS, &pBits2, NULL, 0);
            }
            else
            {
                nNumColors = bi.bmiHeader.biClrUsed;
                hPal = CreatePaletteDx(nNumColors, bi.bmiColors);
                hPalOld = SelectPalette(hMemDC, hPal, FALSE);
                hbm = CreateDIBSection(hMemDC, (BITMAPINFO*)&bi, 
                                       DIB_RGB_COLORS, &pBits2, NULL, 0);
                SelectPalette(hMemDC, hPalOld, FALSE);
                DeleteObject(hPal);
            }
            if (hbm != NULL)
            {
                if (bi.bmiHeader.biBitCount < 8)
                {
                    hbmOld = SelectObject(hMemDC, hbm);
                    SetDIBColorTable(hMemDC, 0, nNumColors, bi.bmiColors);
                    SelectObject(hMemDC, hbmOld);
                }
                if (SetDIBits(hMemDC, hbm, 0, abs(bi.bmiHeader.biHeight),
                              pBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS))
                {
                    ;
                }
                else
                {
                    dwError = GetLastError();
                    DeleteObject(hbm);
                    hbm = NULL;
                }
            }
            else
                dwError = GetLastError();

            DeleteDC(hMemDC);
        }
        else
            dwError = GetLastError();

        ReleaseDC(NULL, hDC);
    }
    else
        dwError = GetLastError();

    HeapFree(GetProcessHeap(), 0, pBits);
    SetLastError(dwError);

    return hbm;
}

BOOL SaveBitmapToFile(LPCTSTR pszFileName, HBITMAP hbm)
{
    BOOL f;
    DWORD dwError;
    BITMAPFILEHEADER bf;
    BITMAPINFOEX bi;
    BITMAPINFOHEADER *pbmih;
    DWORD cb;
    DWORD cColors, cbColors;
    HDC hDC;
    HANDLE hFile;
    LPVOID pBits;
    BITMAP bm;
    
    if (!GetObject(hbm, sizeof(BITMAP), &bm))
        return FALSE;
    
    pbmih = &bi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biCompression      = BI_RGB;
    pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;
    
    if (bm.bmBitsPixel < 16)
        cColors = 1 << bm.bmBitsPixel;
    else
        cColors = 0;
    cbColors = cColors * sizeof(RGBQUAD);
    
    bf.bfType = 0x4d42;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    cb = sizeof(BITMAPFILEHEADER) + pbmih->biSize + cbColors;
    bf.bfOffBits = cb;
    bf.bfSize = cb + pbmih->biSizeImage;
    
    pBits = HeapAlloc(GetProcessHeap(), 0, pbmih->biSizeImage);
    if (pBits == NULL)
        return FALSE;
    
    f = FALSE;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        if (GetDIBits(hDC, hbm, 0, bm.bmHeight, pBits, (BITMAPINFO*)&bi, 
            DIB_RGB_COLORS))
        {
            hFile = CreateFile(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | 
                               FILE_FLAG_WRITE_THROUGH, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                f = WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, NULL) &&
                    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &cb, NULL) &&
                    WriteFile(hFile, bi.bmiColors, cbColors, &cb, NULL) &&
                    WriteFile(hFile, pBits, pbmih->biSizeImage, &cb, NULL);
                if (!f)
                    dwError = GetLastError();
                CloseHandle(hFile);
                
                if (!f)
                    DeleteFile(pszFileName);
            }
            else
                dwError = GetLastError();
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);
    }
    else
        dwError = GetLastError();
    
    HeapFree(GetProcessHeap(), 0, pBits);
    SetLastError(dwError);
    return f;
}

BOOL CopyBitmapBits(HBITMAP hbmDst, HBITMAP hbmSrc, INT cx, INT cy)
{
    BOOL f;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;

    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        DWORD dwError = NO_ERROR;
        hMemDC1 = CreateCompatibleDC(hDC);
        if (hMemDC1 != NULL)
        {
            hMemDC2 = CreateCompatibleDC(hDC);
            if (hMemDC2 != NULL)
            {
                hbmOld1 = SelectObject(hMemDC1, hbmSrc);
                hbmOld2 = SelectObject(hMemDC2, hbmDst);
                f = BitBlt(hMemDC2, 0, 0, cx, cy, hMemDC1, 0, 0, SRCCOPY);
                if (!f)
                {
                    dwError = GetLastError();
                }
                SelectObject(hMemDC1, hbmOld1);
                SelectObject(hMemDC2, hbmOld2);

                DeleteDC(hMemDC2);
            } else
                dwError = GetLastError();
            DeleteDC(hMemDC1);
        } else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }
    return f;
}

BOOL CopyStretchedBitmapBits(HBITMAP hbmDst, HBITMAP hbmSrc, INT cx, INT cy, 
                             INT iStretchBltMode)
{
    BOOL f;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;
    BITMAP bm;
    GetObject(hbmSrc, sizeof(BITMAP), &bm);
    
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        DWORD dwError = NO_ERROR;
        hMemDC1 = CreateCompatibleDC(hDC);
        if (hMemDC1 != NULL)
        {
            hMemDC2 = CreateCompatibleDC(hDC);
            if (hMemDC2 != NULL)
            {
                hbmOld1 = SelectObject(hMemDC1, hbmSrc);
                hbmOld2 = SelectObject(hMemDC2, hbmDst);
                SetStretchBltMode(hMemDC2, iStretchBltMode);
                f = StretchBlt(hMemDC2, 0, 0, cx, cy, hMemDC1, 0, 0, 
                               bm.bmWidth, bm.bmHeight, SRCCOPY);
                if (!f)
                {
                    dwError = GetLastError();
                }
                SelectObject(hMemDC1, hbmOld1);
                SelectObject(hMemDC2, hbmOld2);

                DeleteDC(hMemDC2);
            } else
                dwError = GetLastError();
            DeleteDC(hMemDC1);
        } else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }
    return f;
}

HBITMAP Create24BppBitmap(HBITMAP hbm, INT cx, INT cy)
{
    BITMAPINFOHEADER bi;
    HDC hDC;
    LPVOID pBits;
    HBITMAP hbmNew;
    DWORD dwError;

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = 24;
    bi.biCompression    = BI_RGB;

    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hbmNew = CreateDIBSection(hDC, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pBits,
                                  NULL, 0);
        if (hbmNew != NULL)
        {
            if (!CopyBitmapBits(hbmNew, hbm, cx, cy))
            {
                dwError = GetLastError();
                DeleteObject(hbmNew);
                hbmNew = NULL;
            }
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }

    return hbmNew;
}

HBITMAP Create32BppBitmap(HBITMAP hbm, INT cx, INT cy)
{
    BITMAPINFOHEADER bi;
    HDC hDC;
    LPVOID pBits;
    HBITMAP hbmNew;
    DWORD dwError;

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = 32;
    bi.biCompression    = BI_RGB;

    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hbmNew = CreateDIBSection(hDC, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pBits,
                                  NULL, 0);
        if (hbmNew != NULL)
        {
            if (!CopyBitmapBits(hbmNew, hbm, cx, cy))
            {
                dwError = GetLastError();
                DeleteObject(hbmNew);
                hbmNew = NULL;
            }
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }

    return hbmNew;
}

HBITMAP CreateStretched24BppBitmap(HBITMAP hbm, INT cx, INT cy)
{
    BITMAPINFOHEADER bi;
    HDC hDC;
    LPVOID pBits;
    HBITMAP hbmNew;
    DWORD dwError;

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = 24;
    bi.biCompression    = BI_RGB;

    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hbmNew = CreateDIBSection(hDC, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pBits,
                                  NULL, 0);
        if (hbmNew != NULL)
        {
            if (!CopyStretchedBitmapBits(hbmNew, hbm, cx, cy, COLORONCOLOR))
            {
                dwError = GetLastError();
                DeleteObject(hbmNew);
                hbmNew = NULL;
            }
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }

    return hbmNew;
}

HBITMAP CreateStretched32BppBitmap(HBITMAP hbm, INT cx, INT cy)
{
    BITMAPINFOHEADER bi;
    HDC hDC;
    LPVOID pBits;
    HBITMAP hbmNew;
    DWORD dwError;

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = 32;
    bi.biCompression    = BI_RGB;

    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hbmNew = CreateDIBSection(hDC, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pBits,
                                  NULL, 0);
        if (hbmNew != NULL)
        {
            if (!CopyStretchedBitmapBits(hbmNew, hbm, cx, cy, COLORONCOLOR))
            {
                dwError = GetLastError();
                DeleteObject(hbmNew);
                hbmNew = NULL;
            }
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }

    return hbmNew;
}

HBITMAP CopyBitmap(HBITMAP hbm)
{
    return (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}

BOOL GetMaskedDIB(LPMASKEDDIBINFO pmdi, HBITMAP hbm, INT iCorner, COLORREF rgbBack)
{
    BOOL f;
    DWORD dwError;
    BITMAP bm;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;
    MONOBITMAPINFO bi;
    HBITMAP hbmAND;

    hbm = (HBITMAP)CopyBitmap(hbm);
    if (hbm == NULL)
        return FALSE;
    GetObject(hbm, sizeof(BITMAP), &bm);
    pmdi->bm = bm;
    pmdi->bix.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pmdi->bix.bmiHeader.biWidth        = bm.bmWidth;
    pmdi->bix.bmiHeader.biHeight       = bm.bmHeight;
    pmdi->bix.bmiHeader.biPlanes       = 1;
    pmdi->bix.bmiHeader.biBitCount     = bm.bmBitsPixel;
    pmdi->bix.bmiHeader.biCompression  = BI_RGB;
    pmdi->bix.bmiHeader.biClrUsed      = 0;
    pmdi->bix.bmiHeader.biSizeImage    = 0;

    hDC = CreateCompatibleDC(NULL);
    if (hDC != NULL)
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        GetDIBColorTable(hDC, 0, 256, pmdi->bix.bmiColors);
        SelectObject(hDC, hbmOld);
        DeleteDC(hDC);
    }

    pmdi->nColorCount = DIBNumColors(&pmdi->bix);
    pmdi->cbInfo = sizeof(BITMAPINFOHEADER) +
                   pmdi->nColorCount * sizeof(RGBQUAD);

    pmdi->cbXORBits = BytesPerLine(&pmdi->bix.bmiHeader) * bm.bmHeight;
    pmdi->pXORBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbXORBits);
    if (pmdi->pXORBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        SetLastError(dwError);
        return FALSE;
    }

    hbmAND = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
    if (hbmAND == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        SetLastError(dwError);
        return FALSE;
    }

    pmdi->cbANDBits = WIDTHBYTES(bm.bmWidth * 1) * bm.bmHeight;
    pmdi->pANDBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbANDBits);
    if (pmdi->pANDBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        DeleteObject(hbmAND);
        SetLastError(dwError);
        return FALSE;
    }

    f = FALSE;
    pmdi->cbTotal = pmdi->cbInfo + pmdi->cbXORBits + pmdi->cbANDBits;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        dwError = NO_ERROR;
        hMemDC1 = CreateCompatibleDC(hDC);
        if (hMemDC1 != NULL)
        {
            hMemDC2 = CreateCompatibleDC(hDC);
            if (hMemDC2 != NULL)
            {
                hbmOld1 = SelectObject(hMemDC1, hbm);
                hbmOld2 = SelectObject(hMemDC2, hbmAND);
                switch(iCorner)
                {
                case 0:
                    rgbBack = GetPixel(hMemDC1, 0, 0);
                    break;

                case 1:
                    rgbBack = GetPixel(hMemDC1, 0, bm.bmHeight - 1);
                    break;

                case 2:
                    rgbBack = GetPixel(hMemDC1, bm.bmWidth - 1, 0);
                    break;

                case 3:
                    rgbBack = GetPixel(hMemDC1, bm.bmWidth - 1, bm.bmHeight - 1);
                    break;
                }

                for(int y = 0; y < bm.bmHeight; y++)
                {
                    for(int x = 0; x < bm.bmWidth; x++)
                    {
                        if (GetPixel(hMemDC1, x, y) == rgbBack)
                        {
                            SetPixelV(hMemDC2, x, y, RGB(255, 255, 255));
                        }
                        else
                        {
                            SetPixelV(hMemDC2, x, y, 0);
                        }
                    }
                }
                SelectObject(hMemDC1, hbmOld1);
                SelectObject(hMemDC2, hbmOld2);

                hbmOld1 = SelectObject(hMemDC1, hbm);
                for(int x = 0; x < bm.bmWidth; x++)
                {
                    for(int y = 0; y < bm.bmHeight; y++)
                    {
                        if (GetPixel(hMemDC1, x, y) == rgbBack)
                        {
                            SetPixelV(hMemDC1, x, y, 0);
                        }
                    }
                }
                SelectObject(hMemDC1, hbmOld1);

                hbmOld1 = SelectObject(hMemDC1, hbm);
                f = GetDIBits(hMemDC1, hbm, 0, bm.bmHeight, pmdi->pXORBits,
                          (LPBITMAPINFO)&pmdi->bix, DIB_RGB_COLORS);
                if (!f)
                    dwError = GetLastError();
                SelectObject(hMemDC1, hbmOld1);

                if (f)
                {
                    ZeroMemory(&bi, sizeof(MONOBITMAPINFO));
                    bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
                    bi.bmiHeader.biWidth    = bm.bmWidth;
                    bi.bmiHeader.biHeight   = bm.bmHeight;
                    bi.bmiHeader.biPlanes   = 1;
                    bi.bmiHeader.biBitCount = 1;
                    bi.bmiColors[0].rgbBlue     = 0;
                    bi.bmiColors[0].rgbGreen    = 0;
                    bi.bmiColors[0].rgbRed      = 0;
                    bi.bmiColors[0].rgbReserved = 0;
                    bi.bmiColors[1].rgbBlue     = 255;
                    bi.bmiColors[1].rgbGreen    = 255;
                    bi.bmiColors[1].rgbRed      = 255;
                    bi.bmiColors[1].rgbReserved = 0;
                    f = GetDIBits(hMemDC2, hbmAND, 0, bm.bmHeight,
                              pmdi->pANDBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    if (!f)
                        dwError = GetLastError();
                }

                DeleteDC(hMemDC2);
            }
            else
                dwError = GetLastError();
            DeleteDC(hMemDC1);
        }
        else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    }
    else
        dwError = GetLastError();

    DeleteObject(hbm);
    DeleteObject(hbmAND);
    
    if(!f)
    {
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
    }
    else
        pmdi->bix.bmiHeader.biHeight = bm.bmHeight * 2;

    SetLastError(dwError);
    return f;
}

BOOL GetMaskedDIBAlpha(LPMASKEDDIBINFO pmdi, HBITMAP hbm, HBITMAP hbmAlpha)
{
    BOOL f;
    DWORD dwError;
    BITMAP bm;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;
    COLORREF clr;
    MONOBITMAPINFO bi;
    HBITMAP hbmAND;

    GetObject(hbm, sizeof(BITMAP), &bm);
    hbm = Create32BppBitmap(hbm, bm.bmWidth, bm.bmHeight);
    if (hbm == NULL)
        return FALSE;
    GetObject(hbm, sizeof(BITMAP), &bm);
    pmdi->bm = bm;

    pmdi->bix.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pmdi->bix.bmiHeader.biWidth        = bm.bmWidth;
    pmdi->bix.bmiHeader.biHeight       = bm.bmHeight;
    pmdi->bix.bmiHeader.biPlanes       = 1;
    pmdi->bix.bmiHeader.biBitCount     = bm.bmBitsPixel;
    pmdi->bix.bmiHeader.biCompression  = BI_RGB;
    pmdi->bix.bmiHeader.biClrUsed      = 0;
    pmdi->bix.bmiHeader.biSizeImage    = 0;

    hDC = CreateCompatibleDC(NULL);
    if (hDC != NULL)
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        GetDIBColorTable(hDC, 0, 256, pmdi->bix.bmiColors);
        SelectObject(hDC, hbmOld);
        DeleteDC(hDC);
    }

    pmdi->nColorCount = DIBNumColors(&pmdi->bix);
    pmdi->cbInfo = sizeof(BITMAPINFOHEADER) +
                   pmdi->nColorCount * sizeof(RGBQUAD);

    pmdi->cbXORBits = BytesPerLine(&pmdi->bix.bmiHeader) * bm.bmHeight;
    pmdi->pXORBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbXORBits);
    if (pmdi->pXORBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        SetLastError(dwError);
        return FALSE;
    }

    f = FALSE;
    pmdi->cbANDBits = WIDTHBYTES(bm.bmWidth * 1) * bm.bmHeight;
    pmdi->pANDBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbANDBits);
    if (pmdi->pANDBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        SetLastError(dwError);
        return FALSE;
    }

    hbmAND = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
    if (hbmAND == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
        SetLastError(dwError);
        return FALSE;
    }

    hbmAlpha = (HBITMAP)CopyBitmap(hbmAlpha);
    if (hbmAlpha == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        DeleteObject(hbmAND);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
        SetLastError(dwError);
        return FALSE;
    }

    pmdi->cbTotal = pmdi->cbInfo + pmdi->cbXORBits + pmdi->cbANDBits;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hMemDC1 = CreateCompatibleDC(hDC);
        if (hMemDC1 != NULL)
        {
            hMemDC2 = CreateCompatibleDC(hDC);
            if (hMemDC2 != NULL)
            {
                hbmOld1 = SelectObject(hMemDC1, hbmAlpha);
                hbmOld2 = SelectObject(hMemDC2, hbm);
                for(int y = 0; y < bm.bmHeight; y++)
                {
                    for(int x = 0; x < bm.bmWidth; x++)
                    {
                        if (GetPixel(hMemDC1, x, y) == 0)
                        {
                            SetPixelV(hMemDC2, x, y, 0);
                        }
                    }
                }
                SelectObject(hMemDC1, hbmOld1);
                SelectObject(hMemDC2, hbmOld2);

                hbmOld1 = SelectObject(hMemDC1, hbmAlpha);
                hbmOld2 = SelectObject(hMemDC2, hbmAND);
                for(int y = 0; y < bm.bmHeight; y++)
                {
                    for(int x = 0; x < bm.bmWidth; x++)
                    {
                        if (GetPixel(hMemDC1, x, y) == 0)
                        {
                            SetPixelV(hMemDC2, x, y, RGB(255, 255, 255));
                        }
                        else
                        {
                            SetPixelV(hMemDC2, x, y, 0);
                        }
                    }
                }
                SelectObject(hMemDC1, hbmOld1);
                SelectObject(hMemDC2, hbmOld2);

                f = GetDIBits(hMemDC1, hbm, 0, bm.bmHeight, pmdi->pXORBits,
                          (LPBITMAPINFO)&pmdi->bix, DIB_RGB_COLORS);
                if (!f)
                    dwError = GetLastError();

                if (f)
                {
                    hbmOld1 = SelectObject(hMemDC1, hbmAlpha);
                    for(int x = 0; x < bm.bmWidth; x++)
                    {
                        for(int y = bm.bmHeight - 1; y >= 0; y--)
                        {
                            clr = GetPixel(hMemDC1, x, y);
                            BYTE bAlpha = ((INT)GetRValue(clr) + (INT)GetGValue(clr) + (INT)GetBValue(clr)) / 3;
                            LPBYTE(pmdi->pXORBits)[4 * x + 3 + bm.bmWidthBytes * y] = bAlpha;
                        }
                    }
                    SelectObject(hMemDC1, hbmOld1);

                    ZeroMemory(&bi, sizeof(MONOBITMAPINFO));
                    bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
                    bi.bmiHeader.biWidth    = bm.bmWidth;
                    bi.bmiHeader.biHeight   = bm.bmHeight;
                    bi.bmiHeader.biPlanes   = 1;
                    bi.bmiHeader.biBitCount = 1;
                    bi.bmiColors[0].rgbBlue     = 0;
                    bi.bmiColors[0].rgbGreen    = 0;
                    bi.bmiColors[0].rgbRed      = 0;
                    bi.bmiColors[0].rgbReserved = 0;
                    bi.bmiColors[1].rgbBlue     = 255;
                    bi.bmiColors[1].rgbGreen    = 255;
                    bi.bmiColors[1].rgbRed      = 255;
                    bi.bmiColors[1].rgbReserved = 0;
                    f = GetDIBits(hMemDC2, hbmAND, 0, bm.bmHeight, 
                                  pmdi->pANDBits, (BITMAPINFO*)&bi, 
                                  DIB_RGB_COLORS);
                    if (!f)
                        dwError = GetLastError();
                }

                DeleteDC(hMemDC2);
            } else
                dwError = GetLastError();
            DeleteDC(hMemDC1);
        } else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    } else
        dwError = GetLastError();

    DeleteObject(hbm);
    DeleteObject(hbmAND);
    DeleteObject(hbmAlpha);
    
    if (!f)
    {
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
    }
    else
    {
        pmdi->bix.bmiHeader.biHeight = bm.bmHeight * 2;
    }
    SetLastError(dwError);
    return f;
}


BOOL GetMaskedDIBAlpha2(LPMASKEDDIBINFO pmdi, HBITMAP hbm)
{
    BOOL f;
    DWORD dwError;
    BITMAP bm;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;
    MONOBITMAPINFO bi;
    HBITMAP hbmAND;

    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel != 32)
    {
        SetLastError((DWORD)-(LONG)IDS_ALPHA_CH_INVALID);
        return FALSE;
    }
    pmdi->bm = bm;

    pmdi->bix.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pmdi->bix.bmiHeader.biWidth        = bm.bmWidth;
    pmdi->bix.bmiHeader.biHeight       = bm.bmHeight;
    pmdi->bix.bmiHeader.biPlanes       = 1;
    pmdi->bix.bmiHeader.biBitCount     = bm.bmBitsPixel;
    pmdi->bix.bmiHeader.biCompression  = BI_RGB;
    pmdi->bix.bmiHeader.biClrUsed      = 0;
    pmdi->bix.bmiHeader.biSizeImage    = 0;

    hDC = CreateCompatibleDC(NULL);
    if (hDC != NULL)
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        GetDIBColorTable(hDC, 0, 256, pmdi->bix.bmiColors);
        SelectObject(hDC, hbmOld);
        DeleteDC(hDC);
    }

    pmdi->nColorCount = 0;
    pmdi->cbInfo = sizeof(BITMAPINFOHEADER);

    pmdi->cbXORBits = BytesPerLine(&pmdi->bix.bmiHeader) * bm.bmHeight;
    pmdi->pXORBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbXORBits);
    if (pmdi->pXORBits == NULL)
        return FALSE;

    f = FALSE;
    pmdi->cbANDBits = WIDTHBYTES(bm.bmWidth * 1) * bm.bmHeight;
    pmdi->pANDBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbANDBits);
    if (pmdi->pANDBits == NULL)
    {
        dwError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        SetLastError(dwError);
        return FALSE;
    }

    hbmAND = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
    if (hbmAND == NULL)
    {
        dwError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
        SetLastError(dwError);
        return FALSE;
    }

    pmdi->cbTotal = pmdi->cbInfo + pmdi->cbXORBits + pmdi->cbANDBits;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hMemDC1 = CreateCompatibleDC(hDC);
        if (hMemDC1 != NULL)
        {
            hMemDC2 = CreateCompatibleDC(hDC);
            if (hMemDC2 != NULL)
            {
                f = GetDIBits(hMemDC1, hbm, 0, bm.bmHeight, pmdi->pXORBits,
                          (LPBITMAPINFO)&pmdi->bix, DIB_RGB_COLORS);
                if (f)
                {
                    hbmOld2 = SelectObject(hMemDC2, hbmAND);
                    for(int y = 0; y < bm.bmHeight; y++)
                    {
                        for(int x = 0; x < bm.bmWidth; x++)
                        {
                            if (LPBYTE(pmdi->pXORBits)[4 * x + 3 + bm.bmWidthBytes * (bm.bmHeight - y - 1)])
                            {
                                SetPixelV(hMemDC2, x, y, 0);
                            }
                            else
                            {
                                LPBYTE(pmdi->pXORBits)[4 * x + 0 + bm.bmWidthBytes * (bm.bmHeight - y - 1)] = 0;
                                LPBYTE(pmdi->pXORBits)[4 * x + 1 + bm.bmWidthBytes * (bm.bmHeight - y - 1)] = 0;
                                LPBYTE(pmdi->pXORBits)[4 * x + 2 + bm.bmWidthBytes * (bm.bmHeight - y - 1)] = 0;
                                SetPixelV(hMemDC2, x, y, RGB(255, 255, 255));
                            }
                        }
                    }
                    SelectObject(hMemDC2, hbmOld2);

                    ZeroMemory(&bi, sizeof(MONOBITMAPINFO));
                    bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
                    bi.bmiHeader.biWidth    = bm.bmWidth;
                    bi.bmiHeader.biHeight   = bm.bmHeight;
                    bi.bmiHeader.biPlanes   = 1;
                    bi.bmiHeader.biBitCount = 1;
                    bi.bmiColors[0].rgbBlue     = 0;
                    bi.bmiColors[0].rgbGreen    = 0;
                    bi.bmiColors[0].rgbRed      = 0;
                    bi.bmiColors[0].rgbReserved = 0;
                    bi.bmiColors[1].rgbBlue     = 255;
                    bi.bmiColors[1].rgbGreen    = 255;
                    bi.bmiColors[1].rgbRed      = 255;
                    bi.bmiColors[1].rgbReserved = 0;
                    f = GetDIBits(hMemDC2, hbmAND, 0, bm.bmHeight,
                              pmdi->pANDBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    if (!f)
                        dwError = GetLastError();
                }
                else
                    dwError = GetLastError();
                
                DeleteDC(hMemDC2);
            } else
                dwError = GetLastError();
            DeleteDC(hMemDC1);
        } else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);

        SetLastError(dwError);
    } else
        dwError = GetLastError();

    DeleteObject(hbmAND);
    
    if (!f)
    {
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
    }
    else
    {
        pmdi->bix.bmiHeader.biHeight = bm.bmHeight * 2;
    }
    SetLastError(dwError);
    return f;
}

BOOL GetMaskedDIBAlpha3(LPMASKEDDIBINFO pmdi, HBITMAP hbm)
{
    BOOL f;
    DWORD dwError;
    BITMAP bm;
    HDC hDC, hMemDC1, hMemDC2;
    HGDIOBJ hbmOld1, hbmOld2;
    MONOBITMAPINFO bi;
    HBITMAP hbmAND;

    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel != 32)
    {
        SetLastError((DWORD)-(LONG)IDS_ALPHA_CH_INVALID);
        return FALSE;
    }
    f = FALSE;
    pmdi->bm = bm;
    pmdi->nColorCount = 0;
    if (SaveBitmapAsPngFileToMemory(pmdi->png, hbm))
        f = TRUE;
    pmdi->cbTotal = pmdi->png.Size();
    return f;
}

BOOL GetMaskedDIBAlpha4(LPMASKEDDIBINFO pmdi, HBITMAP hbm, const DataStream &ds)
{
    DWORD dwError;
    BITMAP bm;

    GetObject(hbm, sizeof(BITMAP), &bm);
    pmdi->bm = bm;
    pmdi->nColorCount = 0;
    pmdi->png = ds;
    pmdi->cbTotal = pmdi->png.Size();
    return TRUE;
}

BOOL GetMaskedDIBOriginal(LPMASKEDDIBINFO pmdi, HBITMAP hbm)
{
    BOOL f;
    DWORD dwError;
    BITMAP bm;
    HGDIOBJ hbmOld;
    HDC hDC, hMemDC;
    
    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel == 32)
        hbm = Create24BppBitmap(hbm, bm.bmWidth, bm.bmHeight);
    else
        hbm = (HBITMAP)CopyBitmap(hbm);
    if (hbm == NULL)
        return FALSE;
    GetObject(hbm, sizeof(BITMAP), &bm);
    pmdi->bm = bm;
    pmdi->bix.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pmdi->bix.bmiHeader.biWidth        = bm.bmWidth;
    pmdi->bix.bmiHeader.biHeight       = bm.bmHeight;
    pmdi->bix.bmiHeader.biPlanes       = 1;
    pmdi->bix.bmiHeader.biBitCount     = bm.bmBitsPixel;
    pmdi->bix.bmiHeader.biCompression  = BI_RGB;
    pmdi->bix.bmiHeader.biClrUsed      = 0;
    pmdi->bix.bmiHeader.biSizeImage    = 0;

    hDC = CreateCompatibleDC(NULL);
    if (hDC != NULL)
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        GetDIBColorTable(hDC, 0, 256, pmdi->bix.bmiColors);
        SelectObject(hDC, hbmOld);
        DeleteDC(hDC);
    }

    pmdi->cbXORBits = BytesPerLine(&pmdi->bix.bmiHeader) * bm.bmHeight;
    pmdi->pXORBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbXORBits);
    if (pmdi->pXORBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        SetLastError(dwError);
        return FALSE;
    }

    f = FALSE;
    hDC = GetDC(NULL);
    if (hDC != NULL)
    {
        hMemDC = CreateCompatibleDC(hDC);
        if (hMemDC != NULL)
        {
            f = GetDIBits(hMemDC, hbm, 0, bm.bmHeight, pmdi->pXORBits,
                      (LPBITMAPINFO)&pmdi->bix, DIB_RGB_COLORS);
            if (!f)
                dwError = GetLastError();
            pmdi->nColorCount = DIBNumColors(&pmdi->bix);
            pmdi->cbInfo = sizeof(BITMAPINFOHEADER) +
                           pmdi->nColorCount * sizeof(RGBQUAD);
            DeleteDC(hMemDC);
        } else
            dwError = GetLastError();
        ReleaseDC(NULL, hDC);
    } else
        dwError = GetLastError();

    DeleteObject(hbm);
    
    if (!f)
    {
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        SetLastError(dwError);
        return f;
    }

    pmdi->cbANDBits = WIDTHBYTES(bm.bmWidth * 1) * bm.bmHeight;
    pmdi->pANDBits = HeapAlloc(GetProcessHeap(), 0, pmdi->cbANDBits);
    if (pmdi->pANDBits == NULL)
    {
        dwError = GetLastError();
        DeleteObject(hbm);
        HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
        SetLastError(dwError);
        return FALSE;
    }
    FillMemory(pmdi->pANDBits, pmdi->cbANDBits, RGB(0, 0, 0));

    pmdi->cbTotal = pmdi->cbInfo + pmdi->cbXORBits + pmdi->cbANDBits;
    pmdi->bix.bmiHeader.biHeight       = bm.bmHeight * 2;

    return f;
}

BOOL GetMaskedDIBOriginal2(LPMASKEDDIBINFO pmdi, HBITMAP hbm)
{
    BOOL f;
    BITMAP bm;

    GetObject(hbm, sizeof(BITMAP), &bm);
    f = FALSE;
    pmdi->bm = bm;
    pmdi->nColorCount = 0;
    if (SaveBitmapAsPngFileToMemory(pmdi->png, hbm))
        f = TRUE;
    pmdi->cbTotal = pmdi->png.Size();
    return f;
}

BOOL GetMaskedDIBOriginal3(LPMASKEDDIBINFO pmdi, HBITMAP hbm, const DataStream &ds)
{
    DWORD dwError;
    BITMAP bm;

    GetObject(hbm, sizeof(BITMAP), &bm);
    pmdi->bm = bm;
    pmdi->nColorCount = 0;
    pmdi->png = ds;
    pmdi->cbTotal = pmdi->png.Size();
    return TRUE;
}

VOID DeleteMaskedDIB(LPMASKEDDIBINFO pmdi)
{
    HeapFree(GetProcessHeap(), 0, pmdi->pXORBits);
    HeapFree(GetProcessHeap(), 0, pmdi->pANDBits);
}

static void APIENTRY MyAlphaBlendBlt(
    LPBITMAP pbmDest, INT x, INT y, INT cx, INT cy,
    LPBITMAP pbmSrc, INT xSrc, INT ySrc)
{
    register LPBYTE dest, src;
    register INT ccx, ccy;
 
    /* ‚Í‚Ýo‚½—Ìˆæ‚ª‚ ‚ê‚Î•‚Æ‚‚³‚ÆˆÊ’u‚ðC³‚·‚é */
    cx = min(cx, (INT)pbmSrc->bmWidth);
    if (x < 0)
    {
        cx += x;
        xSrc += -x;
        x = 0;
    }
    cy = min(cy, (INT)pbmSrc->bmHeight);
    if (y < 0)
    {
        cy += y;
        ySrc += -y;
        y = 0;
    }
    cx = min(cx, (INT)pbmDest->bmWidth - x);
    cy = min(cy, (INT)pbmDest->bmHeight - y);
 
    /* –³Œø‚È—Ìˆæ‚©’²‚×‚é */
    if (cx <= 0 || x >= pbmDest->bmWidth ||
        cy <= 0 || y >= pbmDest->bmHeight ||
        xSrc >= pbmSrc->bmWidth || ySrc >= pbmSrc->bmHeight)
        return;
 
    src = (LPBYTE)pbmSrc->bmBits + (xSrc << 2) +
          (pbmSrc->bmHeight - ySrc - 1) * pbmSrc->bmWidthBytes;
    ccy = cy;
    dest = (LPBYTE)pbmDest->bmBits + x * 3 +
           (pbmDest->bmHeight - y - 1) * pbmDest->bmWidthBytes;
    while(ccy--)
    {
        ccx = cx;
        while(ccx--)
        {
            CONST BYTE srcalpha = src[3];
            *dest++ += (*src++ - *dest) * srcalpha >> 8;
            *dest++ += (*src++ - *dest) * srcalpha >> 8;
            *dest++ += (*src++ - *dest) * srcalpha >> 8;
            src++;
        }
        dest -= pbmDest->bmWidthBytes + cx * 3;
        src -= pbmSrc->bmWidthBytes + (cx << 2);
    }
}
 
VOID APIENTRY MyAlphaBlend(
    HDC hdc, INT x, INT y, INT cx, INT cy,
    HDC hdcSrc, INT xSrc, INT ySrc)
{
    HBITMAP hbm, hbmSrc, hbmDummy;
    BITMAP bm, bmSrc;
 
    hbmSrc = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
    GetObject(hbmSrc, sizeof(bmSrc), &bmSrc);
 
    hbmDummy = CreateBitmap(1, 1, 1, 1, NULL);
    if (hbmDummy != NULL)
    {
        hbm = (HBITMAP)SelectObject(hdc, hbmDummy);
        GetObject(hbm, sizeof(bm), &bm);
 
        MyAlphaBlendBlt(&bm, x, y, cx, cy, &bmSrc, xSrc, ySrc);
        DeleteObject(SelectObject(hdc, hbm));
    }
}

HBITMAP CreatePreviewBitmap(HBITMAP hbm, INT cx, INT cy)
{
    BITMAP bm;
    BITMAPINFO bi;
    LPBYTE pbBits;
    HBITMAP hbmNew;
    HDC hdc1, hdc2;

    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel <= 24)
        return CreateStretched24BppBitmap(hbm, cx, cy);
    
    hbm = CreateStretched32BppBitmap(hbm, cx, cy);

    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth        = cx;
    bi.bmiHeader.biHeight       = cy;
    bi.bmiHeader.biPlanes       = 1;
    bi.bmiHeader.biBitCount     = 24;
    bi.bmiHeader.biCompression  = BI_RGB;

    hbmNew = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (VOID **)&pbBits, NULL, 0);
    GetObject(hbmNew, sizeof(BITMAP), &bm);
    for(INT y = 0; y < cy; y++)
    {
        for(INT x = 0; x < cx; x++)
        {
            if (((x >> 3) & 1) ^ (((bm.bmHeight - y - 1) >> 3) & 1))
                pbBits[x * 3 + 0 + y * bm.bmWidthBytes] = 
                pbBits[x * 3 + 1 + y * bm.bmWidthBytes] = 
                pbBits[x * 3 + 2 + y * bm.bmWidthBytes] = 0x88;
            else
                pbBits[x * 3 + 0 + y * bm.bmWidthBytes] = 
                pbBits[x * 3 + 1 + y * bm.bmWidthBytes] = 
                pbBits[x * 3 + 2 + y * bm.bmWidthBytes] = 0xFF;
        }
    }

    hdc1 = CreateCompatibleDC(NULL);
    hdc2 = CreateCompatibleDC(NULL);
    if (hdc1 && hdc2)
    {
        HGDIOBJ hbm1Old, hbm2Old;
        hbm1Old = SelectObject(hdc1, hbm);
        hbm2Old = SelectObject(hdc2, hbmNew);
        MyAlphaBlend(hdc2, 0, 0, cx, cy, hdc1, 0, 0);
        SelectObject(hdc1, hbm1Old);
        SelectObject(hdc2, hbm2Old);
    }
    DeleteDC(hdc1);
    DeleteDC(hdc2);

    DeleteObject(hbm);
    return hbmNew;
}
