#include <windows.h>

#include <vector>
#include <string>
#include <new>
using namespace std;

#include "stream.h"
#include "iconmake.h"
#include "pngimage.h"
#include "resource.h"

HINSTANCE g_hInstance;
HWND g_hDlg;
WNDPROC g_pfnOldCtrlProc;

struct COLORINFO
{
    COLORREF rgb;
    LPTSTR pszName;
};

COLORINFO g_aColorInfo[17] =
{
    {RGB(0, 0, 0), TEXT("Black")},
    {RGB(255, 255, 255), TEXT("White")},
    {RGB(128, 128, 128), TEXT("Gray")},
    {RGB(192, 192, 192), TEXT("Silver")},
    {RGB(128, 0, 0), TEXT("Maroon")},
    {RGB(255, 0, 0), TEXT("Red")},
    {RGB(128, 128, 0), TEXT("Olive")},
    {RGB(255, 255, 0), TEXT("Yellow")},
    {RGB(0, 128, 0), TEXT("Green")},
    {RGB(0, 255, 0), TEXT("Lime")},
    {RGB(0, 128, 128), TEXT("Teal")},
    {RGB(0, 255, 255), TEXT("Aqua")},
    {RGB(0, 0, 128), TEXT("Navy")},
    {RGB(0, 0, 255), TEXT("Blue")},
    {RGB(128, 0, 128), TEXT("Purple")},
    {RGB(255, 0, 255), TEXT("Fuchsia")},
    {RGB(0, 0, 0), TEXT("Custom...")}
};

COLORREF g_aCustColors[16] =
{
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255)
};

TCHAR g_aszEdges[4][32];

struct LISTITEM
{
    string m_file;
    HBITMAP m_hbm;
    HBITMAP m_hbmPreview;
    UINT m_id;
    INT m_iCorner;
    INT m_iColor;
    string m_alphafile;
    HBITMAP m_hbmAlpha;
    INT m_ispng;
    DataStream m_ds;

    LISTITEM() {}
    ~LISTITEM() {}
    LISTITEM(LPCTSTR file, HBITMAP hbm)
    {
        BITMAP bm;
        m_file = file;
        m_hbm = hbm;
        //m_hbmPreview = CreateStretched24BppBitmap(hbm, 90, 90);
        m_hbmPreview = CreatePreviewBitmap(hbm, 90, 90);
        m_id = ID_USE_CORNER;
        m_iCorner = 0;
        m_iColor = 0;
        m_alphafile.clear();
        m_hbmAlpha = NULL;
        m_ispng = 0;
    }
    LISTITEM(LPCTSTR file, HBITMAP hbm, const DataStream& ds)
    {
        BITMAP bm;
        m_file = file;
        m_hbm = hbm;
        //m_hbmPreview = CreateStretched24BppBitmap(hbm, 90, 90);
        m_hbmPreview = CreatePreviewBitmap(hbm, 90, 90);
        m_id = ID_USE_CORNER;
        m_iCorner = 0;
        m_iColor = 0;
        m_alphafile.clear();
        m_hbmAlpha = NULL;
        m_ispng = 0;
        m_ds = ds;
    }
    LISTITEM(const LISTITEM& item)
    {
        m_file = item.m_file;
        m_hbm = item.m_hbm;
        m_hbmPreview = item.m_hbmPreview;
        m_id = item.m_id;
        m_iCorner = item.m_iCorner;
        m_iColor = item.m_iColor;
        m_alphafile = item.m_alphafile;
        m_hbmAlpha = item.m_hbmAlpha;
        m_ispng = item.m_ispng;
        m_ds = item.m_ds;
    }
    LISTITEM& operator=(const LISTITEM& item)
    {
        m_file = item.m_file;
        m_hbm = item.m_hbm;
        m_hbmPreview = item.m_hbmPreview;
        m_id = item.m_id;
        m_iCorner = item.m_iCorner;
        m_iColor = item.m_iColor;
        m_alphafile = item.m_alphafile;
        m_hbmAlpha = item.m_hbmAlpha;
        m_ispng = item.m_ispng;
        m_ds = item.m_ds;
        return *this;
    }
};

vector<LISTITEM> g_list;

LRESULT CALLBACK CtrlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_PAINT:
        CallWindowProc(g_pfnOldCtrlProc, hWnd, uMsg, wParam, lParam);
        InvalidateRect(GetDlgItem(g_hDlg, ID_CORNER), NULL, TRUE);
        UpdateWindow(GetDlgItem(g_hDlg, ID_CORNER));
        break;

    default:
        return CallWindowProc(g_pfnOldCtrlProc, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

VOID OnListSelChange(HWND hDlg)
{
    INT i, c;
    
    i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
    c = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCOUNT, 0, 0);
    
    if (c != LB_ERR)
    {
        if (i != LB_ERR)
        {
            if (i > 0)
                EnableWindow(GetDlgItem(hDlg, ID_UP), TRUE);
            else
                EnableWindow(GetDlgItem(hDlg, ID_UP), FALSE);
            
            if (i < c - 1)
                EnableWindow(GetDlgItem(hDlg, ID_DOWN), TRUE);
            else
                EnableWindow(GetDlgItem(hDlg, ID_DOWN), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_UP), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_DOWN), FALSE);
        }
    }
    
    if (c != LB_ERR && c > 0)
    {
        EnableWindow(GetDlgItem(hDlg, ID_CREATE), TRUE);
        EnableWindow(GetDlgItem(hDlg, ID_CLEAR), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, ID_CREATE), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_CLEAR), FALSE);
    }
    
    if (i == LB_ERR)
    {
        EnableWindow(GetDlgItem(hDlg, ID_DELETE), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_CORNER), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_COLOR), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_ALPHA_MASK), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_ALPHA_CH), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_ORIGINAL), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PNG), FALSE);
        SendDlgItemMessage(hDlg, ID_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, 0);
        SendDlgItemMessage(hDlg, ID_CORNER, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hDlg, ID_COLOR, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hDlg, ID_PROPERTY, WM_SETTEXT, 0, (LPARAM)TEXT(""));
        CheckRadioButton(hDlg, ID_USE_CORNER, ID_USE_ORIGINAL, -1);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, ID_DELETE), TRUE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_CORNER), TRUE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_COLOR), TRUE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_ALPHA_MASK), TRUE);
        EnableWindow(GetDlgItem(hDlg, ID_USE_ORIGINAL), TRUE);

        SendDlgItemMessage(hDlg, ID_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)g_list[i].m_hbmPreview);
        CheckRadioButton(hDlg, ID_USE_CORNER, ID_USE_ORIGINAL,
                         g_list[i].m_id);
        SendDlgItemMessage(hDlg, ID_CORNER, CB_SETCURSEL, g_list[i].m_iCorner, 0);
        SendDlgItemMessage(hDlg, ID_COLOR, CB_SETCURSEL, g_list[i].m_iColor, 0);
        BITMAP bm;
        GetObject(g_list[i].m_hbm, sizeof(BITMAP), &bm);
        TCHAR sz[128], szFormat[128];
        LoadString(g_hInstance, IDS_PROPERTY, szFormat, 128);
        wsprintf(sz, szFormat, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
        SendDlgItemMessage(hDlg, ID_PROPERTY, WM_SETTEXT, 0, (LPARAM)sz);

        if (bm.bmBitsPixel == 32)
            EnableWindow(GetDlgItem(hDlg, ID_USE_ALPHA_CH), TRUE);
        else
            EnableWindow(GetDlgItem(hDlg, ID_USE_ALPHA_CH), FALSE);
        
        if (g_list[i].m_id == ID_USE_CORNER)
        {
            EnableWindow(GetDlgItem(hDlg, ID_CORNER), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
        }

        if (g_list[i].m_id == ID_USE_COLOR)
        {
            EnableWindow(GetDlgItem(hDlg, ID_COLOR), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
        }
        
        if (g_list[i].m_id == ID_USE_ALPHA_MASK)
        {
            EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
            g_list[i].m_alphafile.clear();
            if (g_list[i].m_hbmAlpha != NULL)
            {
                DeleteObject(g_list[i].m_hbmAlpha);
                g_list[i].m_hbmAlpha = NULL;
            }
        }
        if (g_list[i].m_id == ID_USE_ALPHA_CH || g_list[i].m_id == ID_USE_ORIGINAL)
        {
            EnableWindow(GetDlgItem(hDlg, ID_PNG), TRUE);
            if (g_list[i].m_ispng)
                CheckDlgButton(hDlg, ID_PNG, BST_CHECKED);
            else
                CheckDlgButton(hDlg, ID_PNG, BST_UNCHECKED);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_PNG), FALSE);
            CheckDlgButton(hDlg, ID_PNG, BST_UNCHECKED);
        }
        SetDlgItemText(hDlg, ID_ALPHA_FILE, g_list[i].m_alphafile.c_str());
    }
}

VOID ShowLastError(HWND hWnd)
{
    TCHAR szMsg[256];
    DWORD dwError = GetLastError();
    if (dwError != NO_ERROR)
    {
        if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, 
                      szMsg, 256, NULL))
        {
            LoadString(g_hInstance, -(LONG)dwError, szMsg, 256);
        }
        MessageBox(hWnd, szMsg, NULL, MB_OK | MB_ICONERROR);
    }
}

VOID AddBitmap(HWND hDlg, LPCTSTR pszFile, HBITMAP hbm)
{
    TCHAR szFileTitle[256];
    GetFileTitle(pszFile, szFileTitle, 256);
    LISTITEM item(szFileTitle, hbm);
    BITMAP bm;
    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel == 32)
    {
        item.m_id = ID_USE_ALPHA_CH;
        if (bm.bmWidth == 256 && bm.bmHeight == 256)
            item.m_ispng = 1;
    }
    g_list.push_back(item);
    INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_ADDSTRING, 0, (LPARAM)szFileTitle);
    SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i, 0);
    OnListSelChange(hDlg);
}

VOID AddPng(HWND hDlg, LPCTSTR pszFile, HBITMAP hbm)
{
    TCHAR szFileTitle[256];
    GetFileTitle(pszFile, szFileTitle, 256);
    LISTITEM item(szFileTitle, hbm);
    BITMAP bm;
    HANDLE hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbFile = GetFileSize(hFile, NULL);
        LPVOID pFile = HeapAlloc(GetProcessHeap(), 0, cbFile);
        if (cbFile != 0xFFFFFFFF)
        {
            if (pFile != NULL)
            {
                DWORD dw;
                if (ReadFile(hFile, pFile, cbFile, &dw, NULL))
                {
                    try
                    {
                        item.m_ds.Append(pFile, cbFile);
                        item.m_ispng |= 2;
                    }
                    catch(bad_alloc)
                    {
                        ;
                    }
                }
                HeapFree(GetProcessHeap(), 0, pFile);
            }
        }
        CloseHandle(hFile);
    }
    
    GetObject(hbm, sizeof(BITMAP), &bm);
    if (bm.bmBitsPixel == 32)
    {
        item.m_id = ID_USE_ALPHA_CH;
        if (bm.bmWidth == 256 && bm.bmHeight == 256)
            item.m_ispng |= 1;
    }
    else if (bm.bmBitsPixel == 24)
    {
        item.m_id = ID_USE_ORIGINAL;
        if (bm.bmWidth == 256 && bm.bmHeight == 256)
            item.m_ispng |= 1;
    }
    g_list.push_back(item);
    INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_ADDSTRING, 0, (LPARAM)szFileTitle);
    SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i, 0);
    OnListSelChange(hDlg);
}

VOID Add(HWND hDlg, LPCTSTR pszFile)
{
#ifndef LR_LOADREALSIZE
#define LR_LOADREALSIZE 128
#endif
    HBITMAP hbm = (HBITMAP)LoadImage(NULL, pszFile, IMAGE_BITMAP,
        0, 0, LR_LOADREALSIZE | LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hbm == NULL)
    {
        hbm = LoadBitmapFromFile(pszFile);
    }
    if (hbm != NULL)
    {
        AddBitmap(hDlg, pszFile, hbm);
        return;
    }
    hbm = LoadPngAsBitmap(pszFile);
    if (hbm != NULL)
    {
        AddPng(hDlg, pszFile, hbm);
    }
    else
    {
        SetLastError((DWORD)-(LONG)IDS_INVALID);
        ShowLastError(hDlg);
    }
}

BOOL OnAdd(HWND hDlg)
{
    OPENFILENAME ofn;
    TCHAR szFile[10000] = TEXT("");
    TCHAR bmp_files[] = TEXT("*.bmp;*.png");
    TCHAR all_files[] = TEXT("*.*");
    TCHAR szFilter[256], szAddTitle[256];
    TCHAR szDir[MAX_PATH], szPath[MAX_PATH];
    LPTSTR p = szFilter, q;
    
    LoadString(g_hInstance, IDS_BITMAP_PNG, p, 256);
    p += lstrlen(p) + 1;
    lstrcpy(p, bmp_files);
    p += lstrlen(p) + 1;
    LoadString(g_hInstance, IDS_ALL_FILES, p, 256);
    p += lstrlen(p) + 1;
    lstrcpy(p, all_files);
    p += lstrlen(p) + 1;
    *p = '\0';
    
    LoadString(g_hInstance, IDS_OPEN_TO_ADD, szAddTitle, 256);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hDlg;
    ofn.hInstance           = g_hInstance;
    ofn.lpstrFilter         = szFilter;
    ofn.lpstrFile           = szFile;
    ofn.nMaxFile            = 10000;
    ofn.lpstrTitle          = szAddTitle;
    ofn.Flags               = OFN_EXPLORER | OFN_FILEMUSTEXIST |
                              OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | 
                              OFN_ALLOWMULTISELECT;
    ofn.lpstrDefExt = TEXT("bmp");

    if (GetOpenFileName(&ofn))
    {
        if(GetFileAttributes(szFile) & FILE_ATTRIBUTE_DIRECTORY)
        {
            p = szFile;
            lstrcpy(szDir, p);
            p += lstrlen(p) + 1;
            while(*p)
            {
                lstrcpy(szPath, szDir);
                q = CharPrev(szPath, szPath + lstrlen(szPath));
                if (*q != TEXT('\\'))
                    lstrcat(szPath, TEXT("\\"));
                lstrcat(szPath, p);
                Add(hDlg, szPath);
                p += lstrlen(p) + 1;
            }
        }
        else
            Add(hDlg, szFile);
        return TRUE;
    }
    else
    {
        switch(CommDlgExtendedError())
        {
        case CDERR_INITIALIZATION:
        case CDERR_MEMALLOCFAILURE:
        case CDERR_MEMLOCKFAILURE:
        case CDERR_LOADSTRFAILURE:
        case CDERR_LOADRESFAILURE:
        case FNERR_SUBCLASSFAILURE:
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;

        case FNERR_BUFFERTOOSMALL:
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;

        case FNERR_INVALIDFILENAME:
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;

        case 0:
            return TRUE;

        default:
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
    }
}

VOID OnDelete(HWND hDlg)
{
    INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
    if(i == LB_ERR)
        return;

    SendDlgItemMessage(hDlg, ID_LIST, LB_DELETESTRING, i, 0);
    INT c = g_list.size();
    DeleteObject(g_list[i].m_hbm);
    DeleteObject(g_list[i].m_hbmPreview);
    if (g_list[i].m_hbmAlpha != NULL)
        DeleteObject(g_list[i].m_hbmAlpha);
    for(INT j = i; j < c - 1; j++)
    {
        g_list[j] = g_list[j + 1];
    }
    g_list.resize(c - 1);
    if (LB_ERR == SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i, 0))
    {
        SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i - 1, 0);
    }
    OnListSelChange(hDlg);
}

VOID OnUp(HWND hDlg)
{
    INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR || i == 0)
        return;

    LISTITEM item;
    LISTITEM& item1 = g_list[i - 1];
    LISTITEM& item2 = g_list[i];
    item = item1;
    item1 = item2;
    item2 = item;
    SendDlgItemMessage(hDlg, ID_LIST, LB_DELETESTRING, i, 0);
    SendDlgItemMessage(hDlg, ID_LIST, LB_DELETESTRING, i - 1, 0);
    SendDlgItemMessage(hDlg, ID_LIST, LB_INSERTSTRING, i - 1, (LPARAM)item1.m_file.c_str());
    SendDlgItemMessage(hDlg, ID_LIST, LB_INSERTSTRING, i, (LPARAM)item2.m_file.c_str());
    SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i - 1, 0);
    OnListSelChange(hDlg);
}

VOID OnDown(HWND hDlg)
{
    INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
    INT c = g_list.size();
    if (i == LB_ERR || i == c - 1)
        return;

    LISTITEM item;
    LISTITEM& item1 = g_list[i];
    LISTITEM& item2 = g_list[i + 1];
    item = item1;
    item1 = item2;
    item2 = item;
    SendDlgItemMessage(hDlg, ID_LIST, LB_DELETESTRING, i + 1, 0);
    SendDlgItemMessage(hDlg, ID_LIST, LB_DELETESTRING, i, 0);
    SendDlgItemMessage(hDlg, ID_LIST, LB_INSERTSTRING, i, (LPARAM)item1.m_file.c_str());
    SendDlgItemMessage(hDlg, ID_LIST, LB_INSERTSTRING, i + 1, (LPARAM)item2.m_file.c_str());
    SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i + 1, 0);
    OnListSelChange(hDlg);
}

BOOL OnCreate(HWND hDlg)
{
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH] = TEXT("*.ico");
    TCHAR ico_files[] = TEXT("*.ico");
    TCHAR all_files[] = TEXT("*.*");
    TCHAR szFilter[256];
    TCHAR szCreateTitle[256];
    LPTSTR p = szFilter;
    
    LoadString(g_hInstance, IDS_ICON_ICO, p, 256);
    p += lstrlen(p) + 1;
    lstrcpy(p, ico_files);
    p += lstrlen(p) + 1;
    LoadString(g_hInstance, IDS_ALL_FILES, p, 256);
    lstrcpy(p, all_files);
    p += lstrlen(p) + 1;
    *p = '\0';
    
    LoadString(g_hInstance, IDS_CREATE_ICON, szCreateTitle, 256);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hDlg;
    ofn.hInstance       = g_hInstance;
    ofn.lpstrFilter     = szFilter;
    ofn.nFilterIndex    = 0;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrTitle      = szCreateTitle;
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
                          OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = TEXT("ico");

    if (!GetSaveFileName(&ofn))
    {
        switch(CommDlgExtendedError())
        {
        case CDERR_INITIALIZATION:
        case CDERR_MEMALLOCFAILURE:
        case CDERR_MEMLOCKFAILURE:
        case CDERR_LOADSTRFAILURE:
        case CDERR_LOADRESFAILURE:
        case FNERR_SUBCLASSFAILURE:
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;

        case FNERR_BUFFERTOOSMALL:
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;

        case FNERR_INVALIDFILENAME:
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;

        case 0:
            return TRUE;

        default:
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
    }

    INT i, c;
    vector<MASKEDDIBINFO> list;
    LISTITEM item;
    BOOL f;
    DWORD dwError;

    c = g_list.size();
    list.resize(c);
    f = TRUE;
    for(i = 0; i < c; i++)
    {
        item = g_list[i];
        list[i].isPNG = item.m_ispng;
        switch(item.m_id)
        {
        case ID_USE_CORNER:
            if (!GetMaskedDIB(&list[i], item.m_hbm, item.m_iCorner, CLR_INVALID))
            {
                f = FALSE;
                goto break_for;
            }
            break;
            
        case ID_USE_COLOR:
            if (!GetMaskedDIB(&list[i], item.m_hbm, -1, g_aColorInfo[item.m_iColor].rgb))
            {
                f = FALSE;
                goto break_for;
            }
            break;

        case ID_USE_ALPHA_MASK:
            if (!GetMaskedDIBAlpha(&list[i], item.m_hbm, item.m_hbmAlpha))
            {
                f = FALSE;
                goto break_for;
            }
            break;

        case ID_USE_ALPHA_CH:
            if (item.m_ispng & 1)
            {
                if (item.m_ispng & 2)
                {
                    if (!GetMaskedDIBAlpha4(&list[i], item.m_hbm, item.m_ds))
                    {
                        f = FALSE;
                        goto break_for;
                    }
                }
                else if (!GetMaskedDIBAlpha3(&list[i], item.m_hbm))
                {
                    f = FALSE;
                    goto break_for;
                }
            }
            else if (!GetMaskedDIBAlpha2(&list[i], item.m_hbm))
            {
                f = FALSE;
                goto break_for;
            }
            break;

        case ID_USE_ORIGINAL:
            if (item.m_ispng & 1)
            {
                if (item.m_ispng & 2)
                {
                    if (!GetMaskedDIBOriginal3(&list[i], item.m_hbm, item.m_ds))
                    {
                        f = FALSE;
                        goto break_for;
                    }
                }
                else if (!GetMaskedDIBOriginal2(&list[i], item.m_hbm))
                {
                    f = FALSE;
                    goto break_for;
                }
            }
            else if (!GetMaskedDIBOriginal(&list[i], item.m_hbm))
            {
                f = FALSE;
                goto break_for;
            }
            break;
        }
    }
break_for:;
    if (!f)
    {
        dwError = GetLastError();
        for(; --i >= 0; )
        {
            DeleteMaskedDIB(&list[i]);
        }
        SetLastError(dwError);
        return FALSE;
    }
    
    HANDLE hFile;
    DWORD dwOffset;
    ICONDIRHEADER id;
    ICONDIRENTRY ide;
    DWORD cbWritten;

    f = TRUE;
    dwError = NO_ERROR;
    hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        id.idReserved = 0;
        id.idType = 1;
        id.idCount = c;
        if (WriteFile(hFile, &id, sizeof(ICONDIRHEADER), &cbWritten, NULL))
        {
            ide.bReserved       = 0;
            ide.wPlanes         = 1;
            ide.dwImageOffset = sizeof(ICONDIRHEADER) + c * sizeof(ICONDIRENTRY);
            for(i = 0; i < c; i++)
            {
                ide.wBitCount       = list[i].bix.bmiHeader.biBitCount;
                ide.bWidth          = list[i].bm.bmWidth;
                ide.bHeight         = list[i].bm.bmHeight;
                ide.bColorCount     = list[i].nColorCount;
                ide.dwBytesInRes    = list[i].cbTotal;

                if(!WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &cbWritten, NULL))
                {
                    f = FALSE;
                    dwError = GetLastError();
                    break;
                }
                ide.dwImageOffset += ide.dwBytesInRes;
            }

            if (f)
            {
                for(i = 0; i < c; i++)
                {
                    if ((list[i].isPNG & 1) == 1 && list[i].png.Size())
                    {
                        if (!WriteFile(hFile, list[i].png.Ptr(), list[i].png.Size(),
                            &cbWritten, NULL))
                        {
                            f = FALSE;
                            dwError = GetLastError();
                            break;
                        }
                    }
                    else
                    {
                        if (!WriteFile(hFile, &list[i].bix, list[i].cbInfo,
                            &cbWritten, NULL) ||
                            !WriteFile(hFile, list[i].pXORBits, list[i].cbXORBits,
                            &cbWritten, NULL) ||
                            !WriteFile(hFile, list[i].pANDBits, list[i].cbANDBits,
                            &cbWritten, NULL))
                        {
                            f = FALSE;
                            dwError = GetLastError();
                            break;
                        }
                    }
                }
            }
        }
        else
            dwError = GetLastError();

        CloseHandle(hFile);
    }
    else
        dwError = GetLastError();

    for(i = 0; i < c; i++)
    {
        DeleteMaskedDIB(&list[i]);
    }
    SetLastError(dwError);

    return f;
}

VOID OnDropFiles(HWND hDlg, HDROP hDrop)
{
    TCHAR szFile[MAX_PATH];
    UINT c, i;
    c = DragQueryFile(hDrop, 0xFFFFFFFF, szFile, MAX_PATH);
    for(i = 0; i < c; i++)
    {
        DragQueryFile(hDrop, i, szFile, MAX_PATH);
        Add(hDlg, szFile);
    }
    DragFinish(hDrop);
}

VOID OnAlphaBrowse(HWND hDlg)
{
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH] = TEXT("*.bmp");
    TCHAR bmp_files[] = TEXT("*.bmp");
    TCHAR all_files[] = TEXT("*.*");
    TCHAR szFilter[256];
    TCHAR szAlpha[256];
    TCHAR szFileTitle[256];
    LPTSTR p = szFilter;
    
    LoadString(g_hInstance, IDS_BITMAP_BMP, p, 256);
    p += lstrlen(p) + 1;
    lstrcpy(p, bmp_files);
    p += lstrlen(p) + 1;
    LoadString(g_hInstance, IDS_ALL_FILES, p, 256);
    p += lstrlen(p) + 1;
    lstrcpy(p, all_files);
    p += lstrlen(p) + 1;
    *p = '\0';
    
    LoadString(g_hInstance, IDS_ALPHA, szAlpha, 256);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hDlg;
    ofn.hInstance           = g_hInstance;
    ofn.lpstrFilter         = szFilter;
    ofn.lpstrFile           = szFile;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrFileTitle      = szFileTitle; 
    ofn.nMaxFileTitle       = 256;
    ofn.lpstrTitle          = szAlpha;
    ofn.Flags               = OFN_EXPLORER | OFN_FILEMUSTEXIST |
                              OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("bmp");

    if (GetOpenFileName(&ofn))
    {
        INT i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
        if (g_list[i].m_hbmAlpha)
        {
            DeleteObject(g_list[i].m_hbmAlpha);
            g_list[i].m_hbmAlpha = NULL;
        }
        g_list[i].m_hbmAlpha = (HBITMAP)LoadImage(NULL, szFile, IMAGE_BITMAP, 
            0, 0, LR_LOADREALSIZE | LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (g_list[i].m_hbmAlpha == NULL)
        {
            g_list[i].m_hbmAlpha = LoadBitmapFromFile(szFile);
        }
        if (g_list[i].m_hbmAlpha == NULL)
        {
            ShowLastError(hDlg);
            return;
        }
        BITMAP bm1, bm2;
        GetObject(g_list[i].m_hbm, sizeof(BITMAP), &bm1);
        GetObject(g_list[i].m_hbmAlpha, sizeof(BITMAP), &bm2);
        if (bm1.bmWidth != bm2.bmWidth || bm1.bmHeight != bm2.bmHeight)
        {
            TCHAR sz[128];
            DeleteObject(g_list[i].m_hbmAlpha);
            g_list[i].m_hbmAlpha = NULL;
            LoadString(g_hInstance, IDS_SIZE_DIFF, sz, 128);
            MessageBox(hDlg, sz, NULL, MB_ICONERROR);
            return;
        }
        g_list[i].m_alphafile = szFileTitle;
        SetDlgItemText(hDlg, ID_ALPHA_FILE, szFileTitle);
    }
}

VOID OnContextMenu(HWND hDlg, HWND hItem, INT x, INT y)
{
    RECT rc;
    HMENU hMenu, hSubMenu;
    
    if (hItem != GetDlgItem(hDlg, ID_LIST))
        return;
    
    if (x == -1 && y == -1)
    {
        GetWindowRect(hItem, &rc);
        x = rc.left;
        y = rc.top;
    }
    else
    {
        POINT ptClient;
        ptClient.x = x;
        ptClient.y = y;
        ScreenToClient(hItem, &ptClient);
        SendMessage(hItem, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(ptClient.x, ptClient.y));
        SendMessage(hItem, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(ptClient.x, ptClient.y));
    }
    
    hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(1));
    hSubMenu = GetSubMenu(hMenu, 0);
    SetForegroundWindow(hDlg);
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0,
                   hDlg, NULL);
    SendMessage(hDlg, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

BOOL CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT i;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        g_hDlg = hDlg;
        for(i = 0; i < 4; i++)
            SendDlgItemMessage(hDlg, ID_CORNER, CB_ADDSTRING, 0, (LPARAM)g_aszEdges[i]);

        for(i = 0; i < 17; i++)
        {
            SendDlgItemMessage(hDlg, ID_COLOR, CB_ADDSTRING, 0, (LPARAM)i);
        }
        SendDlgItemMessage(hDlg, ID_LIST, WM_SETFONT,
                           (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                           TRUE);
        SendDlgItemMessage(hDlg, ID_CORNER, WM_SETFONT,
                           (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                           TRUE);
        SendDlgItemMessage(hDlg, ID_COLOR, WM_SETFONT,
                           (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                           TRUE);
        SendDlgItemMessage(hDlg, ID_CORNER, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hDlg, ID_COLOR, CB_SETCURSEL, 0, 0);
        SetWindowPos(GetDlgItem(hDlg, ID_USE_CORNER),
            GetDlgItem(hDlg, ID_CORNER), 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE);
        g_pfnOldCtrlProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg,
            ID_USE_CORNER), GWLP_WNDPROC, (LONG_PTR)CtrlProc);
        InvalidateRect(GetDlgItem(hDlg, ID_CORNER), NULL, TRUE);
        UpdateWindow(GetDlgItem(hDlg, ID_CORNER));
        OnListSelChange(hDlg);
        SendMessage(hDlg, WM_SETICON, TRUE, 
                    (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(1)));
        SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)LoadImage(g_hInstance, 
            MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, 0));
        DragAcceptFiles(hDlg, TRUE);
        
        int i;
        for(i = 1; i < __argc; i++)
        {
            Add(hDlg, __argv[i]);
        }
        return TRUE;

    case WM_DROPFILES:
        OnDropFiles(hDlg, (HDROP)wParam);
        break;

    case WM_CONTEXTMENU:
        OnContextMenu(hDlg, (HWND)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
        break;

    case WM_SYSCOMMAND:
        if(wParam == SC_CLOSE) DestroyWindow(hDlg);
        break;

    case WM_MEASUREITEM:
        if (ID_COLOR == wParam)
        {
            LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT)lParam;
            HDC hDC = GetDC(NULL);
            SIZE siz;
            SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
            GetTextExtentPoint(hDC, TEXT("Ag"), 2, &siz);
            pmis->itemHeight = siz.cy + 4;
            ReleaseDC(NULL, hDC);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)TRUE);
            return TRUE;
        }
        break;

    case WM_DESTROY:
        SetWindowLongPtr(GetDlgItem(hDlg, ID_USE_CORNER), GWLP_WNDPROC,
            (LONG_PTR)g_pfnOldCtrlProc);
        PostQuitMessage(0);
        break;

    case WM_DRAWITEM:
        if (wParam == ID_COLOR)
        {
            DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;
            if (pdis->itemState & ODS_SELECTED)
            {
                FillRect(pdis->hDC, &pdis->rcItem,
                         (HBRUSH)(COLOR_HIGHLIGHT + 1));
            }
            else if (pdis->itemState & ODS_DISABLED)
            {
                FillRect(pdis->hDC, &pdis->rcItem,
                         (HBRUSH)(COLOR_3DLIGHT + 1));
            }
            else
            {
                FillRect(pdis->hDC, &pdis->rcItem,
                         (HBRUSH)(COLOR_WINDOW + 1));
            }

            HGDIOBJ hbrOld, hPenOld;
            HBRUSH hbr = CreateSolidBrush(g_aColorInfo[(INT)pdis->itemData].rgb);
            HPEN hPen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));

            hbrOld = SelectObject(pdis->hDC, hbr);
            hPenOld = SelectObject(pdis->hDC, hPen);
            Rectangle(pdis->hDC,
                pdis->rcItem.left + 2,
                pdis->rcItem.top + 2,
                pdis->rcItem.left + 14,
                pdis->rcItem.bottom - 2);
            SelectObject(pdis->hDC, hbrOld);
            SelectObject(pdis->hDC, hPenOld);

            SetBkMode(pdis->hDC, TRANSPARENT);
            RECT rc = pdis->rcItem;
            rc.left = pdis->rcItem.left + 16;
            if (pdis->itemState & ODS_SELECTED)
            {
                SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            }
            else
            {
                SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
            }
            DrawText(pdis->hDC,
                g_aColorInfo[(INT)pdis->itemData].pszName, -1,
                &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            if (pdis->itemState & ODS_FOCUS)
            {
                DrawFocusRect(pdis->hDC, &pdis->rcItem);
            }
        }
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case ID_USE_CORNER:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if(i != LB_ERR)
            {
                g_list[i].m_id = ID_USE_CORNER;
                EnableWindow(GetDlgItem(hDlg, ID_CORNER), TRUE);
                EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_PNG), FALSE);
                if (g_list[i].m_hbmAlpha != NULL)
                {
                    DeleteObject(g_list[i].m_hbmAlpha);
                    g_list[i].m_hbmAlpha = NULL;
                }
                g_list[i].m_alphafile.clear();
                SetDlgItemText(hDlg, ID_ALPHA_FILE, TEXT(""));
            }
            break;

        case ID_USE_COLOR:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if(i != LB_ERR)
            {
                g_list[i].m_id = ID_USE_COLOR;
                EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_COLOR), TRUE);
                EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_PNG), FALSE);
                if (g_list[i].m_hbmAlpha != NULL)
                {
                    DeleteObject(g_list[i].m_hbmAlpha);
                    g_list[i].m_hbmAlpha = NULL;
                }
                g_list[i].m_alphafile.clear();
                SetDlgItemText(hDlg, ID_ALPHA_FILE, TEXT(""));
            }
            break;

        case ID_USE_ALPHA_MASK:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if(i != LB_ERR)
            {
                g_list[i].m_id = ID_USE_ALPHA_MASK;
                EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), TRUE);
                EnableWindow(GetDlgItem(hDlg, ID_PNG), FALSE);
            }
            break;

        case ID_USE_ALPHA_CH:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if(i != LB_ERR)
            {
                g_list[i].m_id = ID_USE_ALPHA_CH;
                EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_PNG), TRUE);
                if (g_list[i].m_hbmAlpha != NULL)
                {
                    DeleteObject(g_list[i].m_hbmAlpha);
                    g_list[i].m_hbmAlpha = NULL;
                }
            }
            break;

        case ID_USE_ORIGINAL:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if(i != LB_ERR)
            {
                g_list[i].m_id = ID_USE_ORIGINAL;
                EnableWindow(GetDlgItem(hDlg, ID_CORNER), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_COLOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_ALPHA_BROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_PNG), TRUE);
                if (g_list[i].m_hbmAlpha != NULL)
                {
                    DeleteObject(g_list[i].m_hbmAlpha);
                    g_list[i].m_hbmAlpha = NULL;
                }
                g_list[i].m_alphafile.clear();
                SetDlgItemText(hDlg, ID_ALPHA_FILE, TEXT(""));
            }
            break;

        case ID_CORNER:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
                if(i != LB_ERR)
                {
                    g_list[i].m_id = ID_USE_CORNER;
                    g_list[i].m_iCorner = SendDlgItemMessage(hDlg, ID_CORNER, CB_GETCURSEL, 0, 0);
                }
            }
            break;

        case ID_COLOR:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
                if(i != LB_ERR)
                {
                    g_list[i].m_id = ID_USE_COLOR;
                    g_list[i].m_iColor = SendDlgItemMessage(hDlg, ID_COLOR, CB_GETCURSEL, 0, 0);
                    if (g_list[i].m_iColor == 16)
                    {
                        CHOOSECOLOR cc;
                        ZeroMemory(&cc, sizeof(CHOOSECOLOR));
                        cc.lStructSize  = sizeof(CHOOSECOLOR);
                        cc.hwndOwner    = hDlg; 
                        cc.rgbResult    = g_aColorInfo[16].rgb;
                        cc.lpCustColors = g_aCustColors;
                        cc.Flags        = CC_RGBINIT;
                        if (ChooseColor(&cc))
                        {
                            g_aColorInfo[16].rgb = cc.rgbResult;
                            InvalidateRect(GetDlgItem(hDlg, ID_COLOR), NULL, TRUE);
                        }
                    }
                }
            }
            break;

        case ID_PNG:
            i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
            if (i != LB_ERR)
            {
                if (IsDlgButtonChecked(hDlg, ID_PNG) == BST_CHECKED)
                {
                    g_list[i].m_ispng |= 1;
                }
                else
                {
                    g_list[i].m_ispng &= ~1;
                }
            }
            break;

        case ID_ALPHA_BROWSE:
            OnAlphaBrowse(hDlg);
            break;

        case ID_PREVIEW:
            if (HIWORD(wParam) == STN_CLICKED && IsDlgButtonChecked(hDlg, ID_USE_COLOR))
            {
                INT i, j;
                HWND hStatic = GetDlgItem(hDlg, ID_PREVIEW);
                HDC hDC = GetDC(hStatic);
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hStatic, &pt);
                COLORREF clr = GetPixel(hDC, pt.x, pt.y);
                i = SendDlgItemMessage(hDlg, ID_LIST, LB_GETCURSEL, 0, 0);
                for(j = 0; j < 16; j++)
                {
                    if (g_aColorInfo[j].rgb == clr)
                        break;
                }
                if (g_aColorInfo[j].rgb == clr)
                {
                    g_list[i].m_iColor = j;
                }
                else
                {
                    g_list[i].m_iColor = 16;
                    g_aColorInfo[16].rgb = clr;
                    UpdateWindow(GetDlgItem(hDlg, ID_COLOR));
                }
                ReleaseDC(hStatic, hDC);
                OnListSelChange(hDlg);
            }
            break;

        case ID_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
                OnListSelChange(hDlg);
            break;

        case ID_ADD:
            if (!OnAdd(hDlg)) ShowLastError(hDlg);
            break;

        case ID_DELETE:
            OnDelete(hDlg);
            break;

        case ID_UP:
            OnUp(hDlg);
            break;

        case ID_DOWN:
            OnDown(hDlg);
            break;

        case ID_CLEAR:
            SendDlgItemMessage(hDlg, ID_LIST, LB_RESETCONTENT, 0, 0);
            for(size_t i = 0; i < g_list.size(); i++)
            {
                DeleteObject(g_list[i].m_hbm);
                DeleteObject(g_list[i].m_hbmPreview);
                if (g_list[i].m_hbmAlpha != NULL)
                    DeleteObject(g_list[i].m_hbmAlpha);
            }
            SetDlgItemText(hDlg, ID_ALPHA_FILE, TEXT(""));
            g_list.clear();
            OnListSelChange(hDlg);
            break;

        case ID_CREATE:
            for(size_t i = 0; i < g_list.size(); i++)
            {
                if (g_list[i].m_id == ID_USE_ALPHA_MASK && 
                    g_list[i].m_hbmAlpha == NULL)
                {
                    TCHAR sz[128];
                    SendDlgItemMessage(hDlg, ID_LIST, LB_SETCURSEL, i, 0);
                    LoadString(g_hInstance, IDS_NO_ALPHA, sz, 128);
                    OnListSelChange(hDlg);
                    SetFocus(GetDlgItem(hDlg, ID_ALPHA_BROWSE));
                    MessageBox(hDlg, sz, NULL, MB_ICONERROR);
                    return FALSE;
                }
            }
            
            if (!OnCreate(hDlg)) ShowLastError(hDlg);
            break;

        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        break;
    }
    return FALSE;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT nCmdShow)
{
    HWND hDlg;
    HACCEL hAccel;
    MSG msg;
    BOOL f;
    
    g_hInstance = hInstance;
    for(int i = 0; i < 4; i++)
    {
        LoadString(g_hInstance, i + IDS_UPPER_LEFT, g_aszEdges[i], 32);
    }
    
    hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);
    if (hDlg == NULL)
    {
        return 1;
    }
    
    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(1));
    if (hAccel == NULL)
    {
        return 2;
    }
    
    ShowWindow(hDlg, nCmdShow);
    UpdateWindow(hDlg);
    
    while((f = GetMessage(&msg, NULL, 0, 0)) != 0 && f != -1)
    {
        if (TranslateAccelerator(hDlg, hAccel, &msg))
            continue;
        if (IsWindow(hDlg) && IsDialogMessage(hDlg, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}
