typedef struct tagICONDIRHEADER
{
    WORD            idReserved;   // Reserved
    WORD            idType;       // resource type (1 for icons)
    WORD            idCount;      // how many images?
} ICONDIRHEADER, FAR * LPICONDIRHEADER;

typedef struct tagICONDIRENTRY
{
    BYTE    bWidth;               // Width of the image
    BYTE    bHeight;              // Height of the image (times 2)
    BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE    bReserved;            // Reserved
    WORD    wPlanes;              // Color Planes
    WORD    wBitCount;            // Bits per pixel
    DWORD   dwBytesInRes;         // how many bytes in this resource?
    DWORD   dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, FAR * LPICONDIRENTRY;

typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

typedef struct tagMONOBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[2];
} MONOBITMAPINFO, FAR * LPMONOBITMAPINFO;

typedef struct tagMASKEDDIBINFO
{
    BITMAP bm;
    INT nColorCount;
    
    DWORD cbInfo;
    BITMAPINFOEX bix;
    
    DWORD cbXORBits;
    LPVOID pXORBits;
    
    DWORD cbANDBits;
    LPVOID pANDBits;
    
    INT isPNG;
    DataStream png;
    DWORD cbTotal;
} MASKEDDIBINFO, FAR * LPMASKEDDIBINFO;

HBITMAP LoadBitmapFromFile(LPCTSTR pszFileName);
BOOL SaveBitmapToFile(LPCTSTR pszFileName, HBITMAP hbm);
HBITMAP CreateStretched24BppBitmap(HBITMAP hbm, INT cx, INT cy);
BOOL GetMaskedDIB(LPMASKEDDIBINFO pmdi, HBITMAP hbm, INT iCorner, COLORREF rgbBack);
BOOL GetMaskedDIBAlpha(LPMASKEDDIBINFO pmdi, HBITMAP hbm, HBITMAP hbmAlpha);
BOOL GetMaskedDIBAlpha2(LPMASKEDDIBINFO pmdi, HBITMAP hbm);
BOOL GetMaskedDIBAlpha3(LPMASKEDDIBINFO pmdi, HBITMAP hbm);
BOOL GetMaskedDIBAlpha4(LPMASKEDDIBINFO pmdi, HBITMAP hbm, const DataStream &ds);
BOOL GetMaskedDIBOriginal(LPMASKEDDIBINFO pmdi, HBITMAP hbm);
BOOL GetMaskedDIBOriginal2(LPMASKEDDIBINFO pmdi, HBITMAP hbm);
BOOL GetMaskedDIBOriginal3(LPMASKEDDIBINFO pmdi, HBITMAP hbm, const DataStream &ds);
VOID DeleteMaskedDIB(LPMASKEDDIBINFO pmdi);
HBITMAP CreatePreviewBitmap(HBITMAP hbm, INT cx, INT cy);
