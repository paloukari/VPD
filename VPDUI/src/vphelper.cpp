
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
#include "vphelper.h"

HANDLE GetPrinterInfo(LPBYTE *pPrinterInfo , TCHAR PrinterName[]) {
     HANDLE hPrinter;
     DWORD cbNeeded;
     //PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };
     PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ACCESS_ADMINISTER };
     OpenPrinter(PrinterName,&hPrinter,NULL);
     GetPrinter(hPrinter,2,NULL ,0,&cbNeeded);
     *pPrinterInfo = (LPBYTE)malloc(cbNeeded);
     if(GetPrinter(hPrinter,2,*pPrinterInfo,cbNeeded,&cbNeeded) == FALSE)
          OutputDebugString(L"failure");
     return hPrinter;
}

BOOL CALLBACK DevicePropertiesDialog(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam)
{
     switch (message)
     {
     case WM_NOTIFY:
         switch (((NMHDR FAR *) lParam)->code) 
         {
         case PSN_APPLY:
             {
                  TCHAR PrintMirrorName[256];
                  GetPrintMirrorName(PrintMirrorName, (HANDLE)lParam);
                  RemovePrinterSharing(PrintMirrorName,NULL,FALSE);
             }
             break;
         }
     default:
         return FALSE;
     }
     return TRUE;   
}

LRESULT CALLBACK DocumentProperty(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam)
{
     static BOOL fBlocking = FALSE;
     static HBRUSH hBrushStatic = NULL;

     switch (message)
     {
     case WM_INITDIALOG:
         {
              VPrinterSettings * ps = (VPrinterSettings *)(((PROPSHEETPAGE *)lParam)->lParam);

              SetWindowLongPtr(hDlg , DWLP_USER , (LONG)lParam);
              HWND hwnd = GetDlgItem(hDlg , IDC_PRINTERCOMBO);
              /* 
               * Get the List of Printers to fill up the Printer combo and then set the 
               * printer name to the one in PrinterSettings 
               */
              DWORD dwReturned,dwNeeded;
              EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, NULL,
                      0, &dwNeeded, &dwReturned) ;

              LPBYTE pinfo4 = (LPBYTE)malloc (dwNeeded) ;

              EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, (PBYTE) pinfo4,
                      dwNeeded, &dwNeeded, &dwReturned) ;
              for(DWORD printers = 0 ; printers < dwReturned ; printers++)
              {
                   LPTSTR PrinterName = ((PRINTER_INFO_4 *)pinfo4)[printers].pPrinterName;
                   if(wcscmp(ps->pszPrinterName ,PrinterName)) 
                        SendMessage( (HWND) hwnd,  CB_ADDSTRING, (WPARAM)0, (LPARAM)PrinterName );
              }

              TCHAR *TargetDriverName;
              LPBYTE pBuffer;
              HANDLE hPrinter = GetPrinterInfo(&pBuffer , ps->pszPrinterName );
			  TargetDriverName = ps->ValidDevMode->pdm.PrinterName;
              TCHAR TargetDriverName1[256];
              wcscpy_s(TargetDriverName1 , 1+wcsnlen(TargetDriverName, maxsize), TargetDriverName);
              //     if(!IsSpooler())
              ValidateSetTargetDriver(TargetDriverName1);
              LRESULT match = SendMessage( (HWND) hwnd, CB_FINDSTRINGEXACT, (WPARAM)-1 ,  (LPARAM)TargetDriverName1);
              wcscpy_s(ps->PrinterName , 1+wcsnlen(TargetDriverName1, maxsize), TargetDriverName1);

              SendMessage( (HWND) hwnd, CB_SETCURSEL, (WPARAM)match, (LPARAM)0);
              free(pinfo4);
              /* Fill the Paper combo and set with the value from PrinterSettings */
              VDEVMODE * pDevMode; 
              pDevMode = ps->ValidDevMode;
              //FillPaperCombo(hPrinter , hDlg , pDevMode , ps);
              /* Layout */
              VPDEVMODE *PrivateDevmode = &((VDEVMODE *)(pDevMode))->pdm;
              free(pBuffer);
              ClosePrinter(hPrinter);
              DUMPMSG("initdialog");
         }          
         break;
     case WM_CTLCOLORSTATIC:         
         break;
     case WM_LBUTTONDOWN:
     case WM_MOUSEMOVE:
     case WM_LBUTTONUP:
         {
         }
         return FALSE;
     case WM_COMMAND:
         {
              PROPSHEETPAGE*  pPage = (PROPSHEETPAGE*)GetWindowLongPtr( hDlg, DWLP_USER );
              VPrinterSettings * ps = (VPrinterSettings *)(pPage->lParam);
              switch(LOWORD(wParam))
              {
              case IDC_PRINTERCOMBO:
                  {
                       if(HIWORD(wParam) == CBN_SELCHANGE)
                       {
                            LPBYTE pBuffer;
                            HANDLE hPrinter = GetPrinterInfo(&pBuffer , ps->pszPrinterName);
                            VDEVMODE *pDevMode;
                            pDevMode = ps->ValidDevMode;
                            LRESULT curSel = SendMessage( GetDlgItem(hDlg, IDC_PRINTERCOMBO),
                                    CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            SendMessage( GetDlgItem(hDlg, IDC_PRINTERCOMBO),
                                    CB_GETLBTEXT, (WPARAM)curSel, (LPARAM)ps->PrinterName);                            
                            free(pBuffer);
                            ClosePrinter(hPrinter);

                       }
                  }
                  break;            
              }
         }
         break;
     case WM_NOTIFY:
         switch (((NMHDR FAR *) lParam)->code) 
         {
         case PSN_APPLY:
             {
                  PROPSHEETPAGE*  pPage = (PROPSHEETPAGE*)GetWindowLongPtr( hDlg, DWLP_USER );
                  VPrinterSettings * ps = (VPrinterSettings *)(pPage->lParam);
                  LONG_PTR cpsResult;
                  cpsResult = ps->pfnComPropSheet( ps->hComPropSheet,
                          CPSFUNC_SET_RESULT, (LPARAM)ps->handle, CPSUI_OK );
                  break;
             }

         case PSN_RESET:
             break;

         case PSN_SETACTIVE:
             break;

         default:
             return FALSE;

         }
         break;
     case WM_DESTROY:
         if(hBrushStatic)
              DeleteObject(hBrushStatic);
         break;
     default:
         return FALSE;
     }
     return TRUE;   
}


static UINT CALLBACK PropSheetPageProc(
        HWND hwnd,
        UINT uMsg,
        LPPROPSHEETPAGE ppsp
        )
{
     UINT toReturn = 0;
     switch (uMsg)
     {
     case PSPCB_RELEASE:
         toReturn = 0;
         break;

     case PSPCB_CREATE:
         toReturn = 1;
         break;

     default:
         break;
     }

     return toReturn;
}

void FillInPropertyPage( PROPSHEETPAGE* psp, int idDlg, LPTSTR pszProc, DLGPROC pfnDlgProc, 
        LPARAM ps)
{
     if(psp)
     {
          psp->dwSize = sizeof(PROPSHEETPAGE);
          psp->dwFlags = PSP_USECALLBACK;
          psp->hInstance = GetHMODULE() ;
          psp->pszTemplate = MAKEINTRESOURCE(idDlg);
          psp->pfnDlgProc = pfnDlgProc;
          psp->pszTitle = (LPTSTR)pszProc;
          psp->lParam = ps;
          psp->pfnCallback = PropSheetPageProc;
     }
}

void ValidateSetTargetDriver(TCHAR *TargetDriverName, VDEVMODE *pdm)
{
     PRINTER_DEFAULTS  defaults = {NULL,NULL,PRINTER_ACCESS_USE};
     HANDLE hPrinter;
     OutputDebugString(L"Enter ValidateSetTargetDriverName");
     if(!OpenPrinter(TargetDriverName , &hPrinter , &defaults))
     {
          DWORD dwReturned,dwNeeded;
          EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL,
                  0, &dwNeeded, &dwReturned) ;

          LPBYTE pinfo4 = (LPBYTE)malloc (dwNeeded) ;

          EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                  NULL, 2, (PBYTE) pinfo4,
                  dwNeeded, &dwNeeded, &dwReturned) ;
          PRINTER_INFO_2 *pi = (PRINTER_INFO_2 *)pinfo4;
          TCHAR PrintMirrorName[256];
          int count = 0;
          BOOL inprintmirror = FALSE,inTargetDriver = FALSE;
          for(DWORD i = 0 ; i < dwReturned ; i++)
          {
               //OutputDebugString(pi[i].pDriverName);
               if(!wcscmp(pi[i].pDriverName , L"VirtuaPrinter"))
               {
                    wcscpy_s(PrintMirrorName , 1+wcsnlen(pi[i].pPrinterName, maxsize), pi[i].pPrinterName);
                    inprintmirror = TRUE;
               }
               else
               {
                    wcscpy_s(TargetDriverName , 1+wcsnlen(pi[i].pPrinterName, maxsize), pi[i].pPrinterName);
                    inTargetDriver = TRUE;
               }
               if(inprintmirror == TRUE && inTargetDriver == TRUE)
                    break;
          }
          free(pi);

          // Fill in the PRINTER_DEFAULTS struct to get full permissions.
          PRINTER_DEFAULTS pd;
          ZeroMemory( &pd, sizeof(PRINTER_DEFAULTS) );
          pd.DesiredAccess = PRINTER_ALL_ACCESS;
          OpenPrinter( PrintMirrorName, &hPrinter, &pd );
          if(dwReturned == 1)
          {
               DeletePrinter(hPrinter);
               return;
          }
          for(int i = 2/*8*/ ; i < 9/*10*/ ; i++)
          {
               if(i == 8)
               {
                    GetPrinter( hPrinter, i, NULL, 0, &dwNeeded );
                    LPBYTE pi2 = (LPBYTE)malloc( dwNeeded );
                    GetPrinter( hPrinter, i, (LPBYTE)pi2, dwNeeded, &dwNeeded );
                    wcscpy_s((TCHAR *)((LPBYTE)(((PRINTER_INFO_8 *)pi2)->pDevMode)
                                + sizeof(DEVMODE)) 
                            , 1+wcsnlen(TargetDriverName, maxsize), TargetDriverName);
                    if(!SetPrinter( hPrinter, i, (LPBYTE)pi2, 0 ))
                    {
                         int k = GetLastError();
                    }

                    free(pi2);
               }
               /*
               else if(i == 2)
               {
                    GetPrinter( hPrinter, i, NULL, 0, &dwNeeded );
                    LPBYTE pi2 = (LPBYTE)malloc( dwNeeded );
                    GetPrinter( hPrinter, i, (LPBYTE)pi2, dwNeeded, &dwNeeded );
                    wcscpy_s((TCHAR *)((LPBYTE)(((PRINTER_INFO_2 *)pi2)->pDevMode)
                                + sizeof(DEVMODE)) 
                            , 1+wcsnlen(TargetDriverName, maxsize), TargetDriverName);
                    SetPrinter( hPrinter, i, (LPBYTE)pi2, 0);
                    free(pi2);
               }
               */
          }
          if(pdm)
          {			  
               wcscpy_s(pdm->pdm.PrinterName, 1+wcsnlen(TargetDriverName, maxsize), TargetDriverName);
          }
          SetTargetDriverName(PrintMirrorName, TargetDriverName);
          ClosePrinter(hPrinter);
     }
     else
          ClosePrinter(hPrinter);
     OutputDebugString(L"Leave ValidateSetTargetDriverName");

}

VOID CopyDeviceName(TCHAR toName[], TCHAR fromName[])
{
	wcsncpy_s(toName, (size_t)CCHDEVICENAME, fromName, (size_t)_TRUNCATE);
}

LONG GetDriverPDEVMODESize(HANDLE hRPrinter, TCHAR *TargetDriverName)
{
	return DocumentProperties(0,hRPrinter , TargetDriverName,0,0,0);
}

BOOL GetDriverPDEVMODE(PDEVMODE& pdmTargetDriver, HANDLE hRPrinter, TCHAR *TargetDriverName)
{
	if(hRPrinter == NULL)
		return FALSE;

	LONG sz = GetDriverPDEVMODESize(hRPrinter, TargetDriverName);
	
	pdmTargetDriver = (PDEVMODE)malloc(sz);

	DocumentProperties(0,hRPrinter, TargetDriverName, pdmTargetDriver, 0, DM_OUT_DEFAULT);

	return TRUE;
}

BOOL RemovePrinterSharing( LPTSTR szPrinterName, LPTSTR szShareName, BOOL
        bShare )
{

     HANDLE            hPrinter;
     PRINTER_DEFAULTS   pd;
     DWORD            dwNeeded;
     PRINTER_INFO_2      *pi2;

     // Fill in the PRINTER_DEFAULTS struct to get full permissions.
     ZeroMemory( &pd, sizeof(PRINTER_DEFAULTS) );
     pd.DesiredAccess = PRINTER_ALL_ACCESS;
     if( ! OpenPrinter( szPrinterName, &hPrinter, &pd ) )
     {
          // OpenPrinter() has failed - bail out.
          return FALSE;
     }
     // See how big a PRINTER_INFO_2 structure is.
     if( ! GetPrinter( hPrinter, 2, NULL, 0, &dwNeeded ) )
     {
          if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
          {
               // GetPrinter() has failed - bail out.
               ClosePrinter( hPrinter );
               return FALSE;
          }
     }
     // Allocate enough memory for a PRINTER_INFO_2 and populate it.
     pi2 = (PRINTER_INFO_2 *)malloc( dwNeeded );
     if( pi2 == NULL )
     {
          // malloc() has failed - bail out.
          ClosePrinter( hPrinter );
          return FALSE;
     }
     if( ! GetPrinter( hPrinter, 2, (LPBYTE)pi2, dwNeeded, &dwNeeded ) )
     {
          // Second call to GetPrinter() has failed - bail out.
          free( pi2 );
          ClosePrinter( hPrinter );
          return FALSE;
     }
     // We won't mess with the security on the printer.
     pi2->pSecurityDescriptor = NULL;
     BOOL set = TRUE;
     if(!(pi2->Attributes & PRINTER_ATTRIBUTE_SHARED))
          set = FALSE;
     // If you want to share the printer, set the bit and the name.
     if( bShare )
     {
          pi2->pShareName = szShareName;
          pi2->Attributes |= PRINTER_ATTRIBUTE_SHARED;
     }
     else // Otherwise, clear the bit.
     {
          pi2->Attributes = pi2->Attributes & (~PRINTER_ATTRIBUTE_SHARED);
     }
     // Make the change.
     if(set && ! SetPrinter( hPrinter, 2, (LPBYTE)pi2, 0 ) )
     {
          // SetPrinter() has failed - bail out
          free( pi2 );
          ClosePrinter( hPrinter );
          return FALSE;
     }
     // Clean up.
     free( pi2 );
     ClosePrinter( hPrinter );
     return TRUE;
} 

void ValidateDevMode(VDEVMODE * ValidDevMode , VDEVMODE * inDevMode)
{
     PDEVMODE indm = (PDEVMODE)(inDevMode);
     PDEVMODE validdm = (PDEVMODE)ValidDevMode;
     if(indm)
     {
          if(indm->dmFields & DM_ORIENTATION)
          {
               validdm->dmOrientation = indm->dmOrientation;
               validdm->dmFields |= DM_ORIENTATION;
          }
          if(indm->dmFields & DM_PAPERSIZE)
          {
               validdm->dmPaperSize = indm->dmPaperSize;
               validdm->dmFields |= DM_PAPERSIZE;
          }
          if(indm->dmFields & DM_PRINTQUALITY)
          {
               validdm->dmPrintQuality = indm->dmPrintQuality;
               validdm->dmFields |= DM_PRINTQUALITY;
          }
          if(indm->dmFields & DM_COLOR)
          {
               validdm->dmColor = indm->dmColor;
               validdm->dmFields |= DM_COLOR;
          }

          if(indm->dmFields & DM_DUPLEX)
          {
               validdm->dmDuplex = indm->dmDuplex;
               validdm->dmFields |= DM_DUPLEX;
          }

          if(indm->dmFields & DM_COLLATE)
          {
               validdm->dmCollate = indm->dmCollate;
               validdm->dmFields |= DM_COLLATE;
          }
          if(indm->dmFields & DM_COPIES)
          {
               validdm->dmCollate = indm->dmCopies;
               validdm->dmFields |= DM_COPIES;
          }
          memcpy((void *)&(ValidDevMode->pdm) ,(void *)&(inDevMode->pdm) , sizeof(VPDEVMODE));
     }
}

void GetPrintMirrorName(TCHAR PrintMirrorName[], HANDLE hPrinter)
{
	DWORD dwNeeded = 0 ;
	 BOOL ret = TRUE;
     // See how big a PRINTER_INFO_2 structure is.
     if( ! GetPrinter( hPrinter, 2, NULL, 0, &dwNeeded ) )
     {
          if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
          {
               // GetPrinter() has failed - bail out.
               ClosePrinter( hPrinter );
               return;
          }
     }
     // Allocate enough memory for a PRINTER_INFO_2 and populate it.
     PRINTER_INFO_2* pi2 = (PRINTER_INFO_2 *)malloc( dwNeeded );
     if( pi2 == NULL )
     {
          // malloc() has failed - bail out.
          ClosePrinter( hPrinter );
          return;
     }
     if( ! GetPrinter( hPrinter, 2, (LPBYTE)pi2, dwNeeded, &dwNeeded ) )
     {
          // Second call to GetPrinter() has failed - bail out.
          free( pi2 );
          ClosePrinter( hPrinter );
          return;
     }

	 wcscpy_s(PrintMirrorName , 1+wcsnlen(pi2->pDevMode->dmDeviceName, maxsize), pi2->pDevMode->dmDeviceName);

	 free( pi2 );
/*
     DWORD dwReturned,dwNeeded;
     EnumPrinters (PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL,
             0, &dwNeeded, &dwReturned) ;

     LPBYTE pinfo4 = (LPBYTE)malloc (dwNeeded) ;

     EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
             NULL, 2, (PBYTE) pinfo4,
             dwNeeded, &dwNeeded, &dwReturned) ;
     PRINTER_INFO_2 *pi = (PRINTER_INFO_2 *)pinfo4;
     for(DWORD i = 0 ; i < dwReturned ; i++)
     {
          if(!wcscmp(pi[i].pDriverName , L"VirtuaPrinter"))
          {
               wcscpy_s(PrintMirrorName , 1+wcsnlen(pi[i].pPrinterName, maxsize), pi[i].pPrinterName);
               break;
          }
     }
     free(pi);*/
}

BOOL IsSpooler()
{
     TCHAR FileName[256];  
     GetModuleFileName( NULL,FileName, 256);
     if(wcsstr(FileName , L"spoolsv.exe") || wcsstr(FileName , L"splwow64.exe"))
          return TRUE;
     return FALSE;

}

BOOL IsExplorer()
{
     TCHAR FileName[256];  
     GetModuleFileName( NULL,FileName, 256);
     if(wcsstr(FileName , L"explorer.exe"))
          return TRUE;
     return FALSE;

} 

VOID FillDeviceCaps(HDC hDC, GDIINFO *pGDIInfo , VDEVMODE *pbIn)
{

     ZeroMemory(pGDIInfo, sizeof(GDIINFO));

     int logpixelsx , logpixelsy;
     HDC memdc = GetDC(NULL);
     pGDIInfo->ulLogPixelsX =logpixelsx = GetDeviceCaps(hDC , LOGPIXELSX);
     pGDIInfo->ulLogPixelsY =logpixelsy = GetDeviceCaps(hDC , LOGPIXELSY);

     pGDIInfo->ulVersion    = GetDeviceCaps(hDC , DRIVERVERSION) ;
     pGDIInfo->ulTechnology = GetDeviceCaps(hDC , TECHNOLOGY);
     /* In mms */
     pGDIInfo->ulHorzSize = GetDeviceCaps(hDC , HORZSIZE);
     pGDIInfo->ulVertSize = GetDeviceCaps(hDC , VERTSIZE);

     pGDIInfo->ulHorzRes  = GetDeviceCaps(hDC , HORZRES);
     pGDIInfo->ulVertRes  = GetDeviceCaps(hDC , VERTRES);
     pGDIInfo->szlPhysSize.cx  =  pGDIInfo->ulHorzRes + 2 * GetDeviceCaps(hDC , PHYSICALOFFSETX);
     pGDIInfo->szlPhysSize.cy  = pGDIInfo->ulVertRes + 2 * GetDeviceCaps(hDC , PHYSICALOFFSETY);

     pGDIInfo->ptlPhysOffset.x = GetDeviceCaps(hDC , PHYSICALOFFSETX);
     pGDIInfo->ptlPhysOffset.y = GetDeviceCaps(hDC , PHYSICALOFFSETY);


     //
     // Assume the device has a 1:1 aspect ratio
     //


     pGDIInfo->ulAspectX    = 10;//GetDeviceCaps(memdc , ASPECTX);
     pGDIInfo->ulAspectY    = 10;//GetDeviceCaps(memdc , ASPECTY);
     pGDIInfo->ulAspectXY   = 14;//GetDeviceCaps(memdc , ASPECTXY);

     COLORINFO ciDevice= {
          { 6810, 3050,     0 },  // xr, yr, Yr
          { 2260, 6550,     0 },  // xg, yg, Yg
          { 1810,  500,     0 },  // xb, yb, Yb
          { 2000, 2450,     0 },  // xc, yc, Yc
          { 5210, 2100,     0 },  // xm, ym, Ym
          { 4750, 5100,     0 },  // xy, yy, Yy
          { 3324, 3474, 10000 },  // xw, yw, Yw

          10000,                  // R gamma
          10000,                  // G gamma
          10000,                  // B gamma

          1422,  952,             // M/C, Y/C
          787,  495,             // C/M, Y/M
          324,  248              // C/Y, M/Y
     };

     pGDIInfo->ciDevice        = ciDevice;
     pGDIInfo->ulDevicePelsDPI   =   pGDIInfo->ulLogPixelsX;
     pGDIInfo->ulNumPalReg= 0;
#define HT_FORMAT_24BPP         6
#define HT_PATSIZE_16x16_M      15
#define HT_FLAG_HAS_BLACK_DYE  0x00000002
#define PRIMARY_ORDER_ABC       0
     pGDIInfo->ulHTOutputFormat= HT_FORMAT_24BPP;

     pGDIInfo->ulHTPatternSize   =  HT_PATSIZE_16x16_M;
     pGDIInfo->flHTFlags       = HT_FLAG_HAS_BLACK_DYE;
     pGDIInfo->ulPrimaryOrder  = PRIMARY_ORDER_ABC;

     pGDIInfo->ulNumColors =  GetDeviceCaps(hDC , NUMCOLORS);
     pGDIInfo->cBitsPixel = GetDeviceCaps(hDC , BITSPIXEL);
     pGDIInfo->cPlanes    = GetDeviceCaps(hDC , PLANES);
     //
     // Some other information the Engine expects us to fill in.
     //

     pGDIInfo->ulDACRed     = 0;
     pGDIInfo->ulDACGreen   = 0;
     pGDIInfo->ulDACBlue    = 0;
     pGDIInfo->flRaster     = 0;
     pGDIInfo->flTextCaps   = 0;//GetDeviceCaps(hDC,TEXTCAPS);
     pGDIInfo->xStyleStep   = 1;
     pGDIInfo->yStyleStep   = 1;
     pGDIInfo->denStyleStep  =   pGDIInfo->ulDevicePelsDPI/ 25;
     ReleaseDC(NULL,memdc);

}

DWORD GetTargetDriverName(TCHAR PrinterName[], TCHAR TargetDriverName[])
{

     HANDLE hPrinter;
     PRINTER_DEFAULTS defaults = { NULL, NULL, READ_CONTROL };
     OpenPrinter(PrinterName,&hPrinter,&defaults);
     DWORD cbNeeded;   
     DWORD dwret = GetPrinterData(hPrinter,L"RealDriverName",NULL,
             (LPBYTE)TargetDriverName,256,&cbNeeded);
     ClosePrinter(hPrinter);
     return dwret;
}

DWORD GetTargetDriverName(HANDLE hPrinter, TCHAR TargetDriverName[])
{
     DWORD cbNeeded;
     return GetPrinterData(
             hPrinter,    // handle to printer or print server
             L"RealDriverName",  // value name
             NULL,      // data type
             (LPBYTE)TargetDriverName,       // configuration data buffer
             256,        // size of configuration data buffer
             &cbNeeded   // bytes received or required 
             );
}

DWORD GetTargetFromRegistry(TCHAR TargetDriverName[])
{
	HKEY hKey;
	DWORD  dwMaxLength = 255;
	DWORD dwDataType = REG_SZ;

	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers",
                  0,
                  KEY_QUERY_VALUE,
                  &hKey) == ERROR_SUCCESS)
	{
	  // Get CSD version	  

	  if(::RegQueryValueEx(hKey,
						  TEXT("VPDTarget"),
						  NULL,
						  &dwDataType,
						  (LPBYTE)(TargetDriverName),
						  &dwMaxLength) != ERROR_SUCCESS)
	  {
		// Close key
		::RegCloseKey(hKey);

		// Error handling;
		return -1;
	  }

	  // Close key
	  ::RegCloseKey(hKey);
	  return 0;
	}
	else
	{
	  // Error handling;
		return -1;
	}
}

DWORD SetTargetDriverName(TCHAR PrinterName[], TCHAR TargetDriverName[])
{
     HANDLE hPrinter;
     PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };

     OpenPrinter(
             PrinterName,         // printer or server name
             &hPrinter,          // printer or server handle
             &defaults   // printer defaults
             );
     DWORD dwret = SetPrinterData(
             hPrinter,    // handle to printer or print server
             L"RealDriverName",  // data to set
             REG_SZ,         // data type
             (LPBYTE)TargetDriverName,       // configuration data buffer
             (wcslen(TargetDriverName) + 1) * sizeof(TCHAR)// size of buffer
             );
     ClosePrinter(hPrinter);
     return dwret;

}

TCHAR *GetTempFile(TCHAR *TempPath , TCHAR *Prefix,TCHAR *TempFileName)
{
     BOOL isTempPathNull = FALSE;
     if(TempPath == NULL)
     {
          TempPath = (TCHAR *)malloc(sizeof(TCHAR) * MAX_PATH);
          GetTempPath(
                  MAX_PATH,  // size of buffer
                  TempPath        // path buffer
                  );

          isTempPathNull = TRUE;
     }
     GetTempFileName(
             TempPath,      // directory name
             Prefix,  // file name prefix
             0,            // integer
             TempFileName    // file name buffer
             );

     if(isTempPathNull == TRUE)
          free(TempPath);
     return TempFileName;
}

PDEVMODE GetPrinterSettings(PTSTR lpszDeviceName, LPBYTE printerSettings)
{
	HANDLE hPrinterNew = NULL;                    
	DWORD cbNeeded , dwret = -1;   
	PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };

	OpenPrinter(lpszDeviceName,&hPrinterNew,&defaults);
    
    if(GetPrinterData(hPrinterNew,L"PrinterSettings",NULL,
		(LPBYTE)printerSettings,0,&cbNeeded) == ERROR_MORE_DATA)
	{
		printerSettings = (LPBYTE)malloc(cbNeeded);

		dwret = GetPrinterData(hPrinterNew,L"PrinterSettings",NULL,
			(LPBYTE)printerSettings,cbNeeded,&cbNeeded);		
	}

	ClosePrinter(hPrinterNew);

	if(dwret == ERROR_SUCCESS)
		return (PDEVMODE)printerSettings;	

	free(printerSettings);
            
	return NULL;
}

int GetColorOrganisation(HDC hTargetDC , DEVMODE *pbIn , ULONG palette[])
{

     int ret = 0;
     HDC compDC = CreateCompatibleDC(hTargetDC);
     HBITMAP hBitmap = CreateCompatibleBitmap(hTargetDC , 1 , 1);
     SelectObject(compDC , hBitmap);
     BITMAP   bmp;
     GetObject(hBitmap ,sizeof(BITMAP), &bmp);
     if(bmp.bmBitsPixel <= 8)
     {
          switch(bmp.bmBitsPixel)
          {
          case 8:
              {
                   BYTE bits = 0;
                   for(int i = 0 ; i < 256 ; i++)
                   {
                        bits = (BYTE)i;
                        SetBitmapBits(hBitmap , 1  , &bits);
                        palette[i] =  GetPixel(compDC , 0 , 0 );
                   }
                   ret = 256;
              }
              break;
          case 1:
              {
                   BYTE bits = 0;
                   for(int i = 0 ; i < 2 ; i++)
                   {
                        SetBitmapBits(hBitmap , 1  , &bits);
                        bits = ~bits;
                        palette[i] =  GetPixel(compDC , 0 , 0 );
                   }
                   ret = 2;
              }
              break;
          case 4:
              {
                   BYTE bits = 0;
                   for(int i = 0 ; i < 16 ; i++)
                   {
                        bits = (BYTE)(i << 4);
                        SetBitmapBits(hBitmap , 1  , &bits);
                        palette[i] =  GetPixel(compDC , 0 , 0 );
                   }
                   ret = 16;
              }
              break;
          }
     }
     DeleteDC(compDC);
     DeleteObject(hBitmap);
     return ret;
}

void CreateGDIInfo(HANDLE hPrinter,VDEVMODE *pbIn)
{
     WCHAR TargetDriverName[256];
     wcscpy_s(TargetDriverName , 1+wcsnlen(pbIn->pdm.PrinterName, maxsize),pbIn->pdm.PrinterName);
     /* Get the real driver's devmode and hack with our's */
     HANDLE hRPrinter;   
     OpenPrinter(TargetDriverName, &hRPrinter, NULL);
     LONG sz = DocumentProperties(0,hRPrinter , TargetDriverName,0,0,0);

     PDEVMODE  pdmInput1 = NULL;
     PDEVMODE  pdmOutput1 = NULL;
     pdmInput1 = (PDEVMODE)malloc(sz);
     pdmOutput1 = (PDEVMODE)malloc(sz);
     DocumentProperties(0,hRPrinter , TargetDriverName,pdmInput1,0,DM_OUT_DEFAULT);
     WORD dmDriverExtra = pdmInput1->dmDriverExtra;
     memcpy(pdmInput1,pbIn,sizeof(DEVMODEW));
     pdmInput1->dmDriverExtra = dmDriverExtra;
     DocumentProperties(0,hRPrinter , TargetDriverName,pdmOutput1,pdmInput1,DM_OUT_BUFFER |
             DM_IN_BUFFER);

     ClosePrinter(hRPrinter);
     HDC hTargetDC =  CreateDC(L"WINSPOOL",TargetDriverName,NULL , pdmOutput1);
     free(pdmInput1);
     free(pdmOutput1);

     FillDeviceCaps(hTargetDC,&(pbIn->pdm.gi),pbIn);
     ULONG palette[256];
     int numcolors = GetColorOrganisation(hTargetDC , (DEVMODE *)pbIn, palette );
     pbIn->pdm.numcolors = numcolors;
     memcpy(pbIn->pdm.Palette , palette , sizeof(ULONG) * 256);
     LOGFONT lf;
     HGDIOBJ hFont = GetCurrentObject(hTargetDC,OBJ_FONT);
     GetObject(hFont , sizeof(LOGFONT), &lf);
     memcpy(&(pbIn->pdm.lf) , &lf , sizeof(LOGFONT));

     DeleteDC(hTargetDC);
}

void CreateWin2kcompatibleSplFile(HANDLE hPrinter)
{

     LPBYTE pPInfo;
     DWORD cbNeeded = 0 ;
	 BOOL ret = TRUE;
     ret = GetPrinter( hPrinter, 2, NULL , 0, &cbNeeded);
     pPInfo = (LPBYTE)malloc(cbNeeded);
     ret = GetPrinter( hPrinter, 2, pPInfo, cbNeeded, &cbNeeded );
     if(!(((PRINTER_INFO_2 *)pPInfo)->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS)
             ||
             (((PRINTER_INFO_2 *)pPInfo)->Attributes & PRINTER_ATTRIBUTE_DIRECT))
     {
          ((PRINTER_INFO_2 *)pPInfo)->Attributes |=PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS;
          ((PRINTER_INFO_2 *)pPInfo)->Attributes &= ~PRINTER_ATTRIBUTE_DIRECT;
		  ((PRINTER_INFO_2 *)pPInfo)->Attributes |= PRINTER_ATTRIBUTE_QUEUED;  
          PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ACCESS_ADMINISTER};
          HANDLE hDriver;
          OpenPrinter( ((PRINTER_INFO_2 *)pPInfo)->pPrinterName, &hDriver, &defaults);
          if(!SetPrinter( hDriver, 2, pPInfo , 0 ))
          {
          }
          ClosePrinter(hDriver);
     }
     free(pPInfo);
}

unsigned long __stdcall ThreadFunc( LPVOID lpParam ) 
{ 
     HANDLE hPrinter = *((HANDLE *)lpParam);
     DWORD dwJobId = *(DWORD *)((LPBYTE)lpParam + sizeof(HANDLE));
     Sleep(5000);
     SetJob(hPrinter , dwJobId,0,0,JOB_CONTROL_DELETE);
     ClosePrinter(hPrinter); 
     free(lpParam);
     return 0; 
} 

