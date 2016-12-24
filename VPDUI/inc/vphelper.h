#ifndef _VPRINTERHELPERS_H
#define _VPRINTERHELPERS_H
#include "precomp.h"
#include <stdlib.h> 
#include <malloc.h>
#include "resourceui.h"
#pragma hdrstop
extern size_t maxsize;
extern "C"
{
#include <windowsx.h>
}
#include "vpui.h"

DWORD DrvDeviceCapabilities(HANDLE  hPrinter,PWSTR  pDeviceName, WORD  iDevCap, VOID  *pvOutput,DEVMODE  *pDevMode);
HMODULE GetHMODULE();


DWORD GetRealDriverName(WCHAR PrinterName[], WCHAR RealDriverName[]);
DWORD GetRealDriverName(HANDLE hPrinter, WCHAR RealDriverName[]);
DWORD SetRealDriverName(WCHAR PrinterName[], WCHAR RealDriverName[]);

DWORD GetTargetFromRegistry(TCHAR TargetDriverName[]);


void GetMetaFileFromSpoolFile(TCHAR *SpoolFileName , LONG_PTR PageNbr , TCHAR *MetaFileName, PPDEV pPDev,LPBYTE *pDevmode);
void GetSpoolFileName(DWORD JobId, TCHAR SpoolFileName[],HANDLE hDriver);
void SaveAsBitmap(HWND hDlg , OPENFILENAME ofn , PPDEV pPDev);
void PrintToPaper(PPDEV pPDev);


BOOL CALLBACK DevicePropertiesDialog(HWND hDlg,UINT message,UINT wParam,LONG lParam);
LRESULT CALLBACK DocumentProperty(HWND hDlg,UINT message,UINT wParam,LONG lParam);
UINT CALLBACK PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
void FillInPropertyPage( PROPSHEETPAGE* psp, int idDlg, LPTSTR pszProc, DLGPROC pfnDlgProc, LPARAM ps);
void ValidateSetTargetDriver(TCHAR *TargetDriverName, VDEVMODE *pdm = NULL);
LONG GetDriverPDEVMODESize(HANDLE hRPrinter, TCHAR *TargetDriverName);
BOOL GetDriverPDEVMODE(PDEVMODE& pdmTargetDriver, HANDLE hRPrinter, TCHAR *TargetDriverName);
BOOL RemovePrinterSharing( LPTSTR szPrinterName, LPTSTR szShareName, BOOL bShare );
void ValidateDevMode(VDEVMODE * ValidDevMode , VDEVMODE * inDevMode);
void GetPrintMirrorName(TCHAR PrintMirrorName[], HANDLE hPrinter);
BOOL IsSpooler();
BOOL IsExplorer();
VOID FillDeviceCaps(HDC hDC, GDIINFO *pGDIInfo , VDEVMODE *pbIn);
HANDLE GetPrinterInfo(LPBYTE *pPrinterInfo , TCHAR PrinterName[]);
DWORD GetTargetDriverName(TCHAR PrinterName[], TCHAR TargetDriverName[]);
DWORD GetTargetDriverName(HANDLE hPrinter, TCHAR TargetDriverName[]);
DWORD SetTargetDriverName(TCHAR PrinterName[], TCHAR TargetDriverName[]);
TCHAR *GetTempFile(TCHAR *TempPath , TCHAR *Prefix,TCHAR *TempFileName);
BOOL IsInchDimensions();
PDEVMODE GetPrinterSettings(PTSTR lpszDeviceName, LPBYTE printerSettings);
void CreateWin2kcompatibleSplFile(HANDLE hPrinter);
void CreateGDIInfo(HANDLE hPrinter,VDEVMODE *pbIn);
unsigned long __stdcall ThreadFunc( LPVOID lpParam ) ;
VOID CopyDeviceName(TCHAR toName[], TCHAR fromName[]);

DLGPROC APIENTRY PMDialog(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam);

DLGPROC APIENTRY LicenseDialog(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam);


BOOL PopFileSaveDlg(HWND hwnd , LPTSTR pstrFileName , LPTSTR pstrTitleName , OPENFILENAME &ofn);
#endif