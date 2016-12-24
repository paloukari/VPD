#ifndef _VPUI_H
#define _VPUI_H
#define PDEV_ESCAPE 0x303eb8efU
#define GDIINFO_ESCAPE 0x303eb9efU
#define MALLOC malloc


#include "resourceui.h"

typedef enum Layout { UP1 ,UP2,UP4,UP6,UP9,BOOKLET} Layout;

typedef LONG  LDECI4;


struct PageSize{
     WORD dmPaperSize;
     POINT pt;
};

typedef struct VPDEVMODE{
     WCHAR PrinterName[256];
     Layout lyt;
     WORD PaperSize;
     GDIINFO  gi;
     int numcolors;
     ULONG Palette[256];
     LOGFONT lf;
	 BOOL Preview;
	 BOOL PrintToPaper;
     BYTE CompLevel;
} VPDEVMODE;

typedef struct VDEVMODE{
     DEVMODEW dm;
     VPDEVMODE pdm;
} VDEVMODE;

typedef struct DEVDATA{
    DWORD dwJobId;
    int Pages;
    WCHAR *pSpoolFileName;
    HANDLE hPDriver;
    BOOL *pResetDC;
	HBITMAP hBitmap;
	LPVOID pvBits;
    VDEVMODE *pCurDevmode;
    LPTSTR pDocument;
} DEVDATA;

#define PPDEV DEVDATA*

typedef struct VPrinterSettings {
     WCHAR PrinterName[256];
     VDEVMODE *inDevmode;
     VDEVMODE *ValidDevMode;
     VDEVMODE *outDevmode;
     PFNCOMPROPSHEET  pfnComPropSheet;
     HANDLE  hComPropSheet;
     HANDLE handle;
     HANDLE hPrinter;
     LPTSTR pszPrinterName; 
     DWORD fMode;
     Layout lyt;
     BYTE  Render;
     
     WCHAR *Test;
	 BOOL DimensionUnits;
	 
	 BOOL Preview;
	 BOOL PrintToPaper;
     BYTE CompLevel;
} VPrinterSettings;


typedef struct Token{
     int TokenStart;
     int TokenEnd;
     struct Token *NextToken;
} Token;


class VPDriver{

     BOOL bIsExplorer;          
     PPDEV pPDevG;
     TCHAR PrinterName[MAX_PATH]; 
     WCHAR TargetPrinterName[256];
     LONG DrvDocumentProperties(HWND hwnd, HANDLE hPrinter, PTSTR lpszDeviceName,
             PDEVMODE pdmOutput,PDEVMODE pdmInput, DWORD fMode,BOOL fromApp = FALSE);
     void FixUpDevmode(IN HANDLE hPrinter , IN DOCEVENT_CREATEDCPRE *pbIn, IN OUT PULONG pbOut);
    public:
     VPDriver(BOOL bIsExp);

	 
     DWORD  PMDrvDeviceCapabilities(HANDLE  hPrinter,PWSTR  pDeviceName, WORD  iDevCap,
             VOID  *pvOutput,DEVMODE  *pDevMode);
     INT DrvDocumentEventImpl( HANDLE  hPrinter, HDC  hdc, int  iEsc, ULONG  cbIn, PULONG  pbIn,
             ULONG  cbOut, PULONG  pbOut); 
     LONG  DrvDocumentPropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam);
     BOOL  DrvPrinterEvent(LPWSTR  pPrinterName, INT  DriverEvent, DWORD  Flags, LPARAM  lParam);
     LONG  DrvDevicePropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam);
     BOOL  DevQueryPrintEx(PDEVQUERYPRINT_INFO  pDQPInfo);
     BOOL  DrvConvertDevMode(LPTSTR  pPrinterName, PDEVMODE  pdmIn, PDEVMODE  pdmOut, PLONG  pcbNeeded,DWORD  fMode);
};
#endif