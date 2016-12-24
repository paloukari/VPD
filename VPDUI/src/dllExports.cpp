
extern "C" {
#include "precomp.h"
}
#pragma hdrstop

size_t maxsize = 1024;

extern "C"
{
#include "windowsx.h"
}
#include "vphelper.h"

#define DBG_PROCESS_ATTACH  0x00000001
#define DBG_PROCESS_DETACH  0x00000002

#define CCHBINNAME          24      // Characters allowed for bin names
#define CCHPAPERNAME        64      // Max length of paper size names
#define DC_SPL_PAPERNAMES   0xFFFF
#define DC_SPL_MEDIAREADY   0xFFFE


extern "C" HMODULE hPlotUIModule = NULL;

HMODULE GetHMODULE()
{
	return hPlotUIModule;
}

VPDriver *vpui = NULL;


BOOL
DllMain(
    HINSTANCE   hModule,
    DWORD       Reason,
    LPVOID      pReserved
    )
{
	DUMPMSG("DllMain");

    UNREFERENCED_PARAMETER(pReserved);
	
	
    switch (Reason) {

    case DLL_PROCESS_ATTACH:
		{
        DUMPMSG("DLL_PROCESS_ATTACH:");

		BOOL bIsExplorer = FALSE;
        hPlotUIModule = hModule;

        WCHAR wName[MAX_PATH];
        if (GetModuleFileName(hPlotUIModule, wName, MAX_PATH)) 
            LoadLibrary(wName);
		
        bIsExplorer =  IsExplorer();
        vpui = new VPDriver(bIsExplorer);              
		}
        break;

    case DLL_PROCESS_DETACH:

        DUMPMSG("DBG_PROCESS_DETACH");
        break;
    }

    return(TRUE);
}


DWORD DrvDeviceCapabilities(HANDLE  hPrinter,PWSTR  pDeviceName, WORD  iDevCap,
        VOID  *pvOutput,DEVMODE  *pDevMode)
{
	DUMPMSG("DrvDeviceCapabilities");
    return vpui->PMDrvDeviceCapabilities(hPrinter,pDeviceName,iDevCap,pvOutput,pDevMode);  
}

INT DrvDocumentEvent( HANDLE  hPrinter, HDC  hdc, int  iEsc, ULONG  cbIn, PULONG  pbIn,
        ULONG  cbOut, PULONG  pbOut)
{
	DUMPMSG("DrvDocumentEvent");
    return vpui->DrvDocumentEventImpl(hPrinter,hdc,iEsc,cbIn,pbIn,cbOut,pbOut);
}

LONG  DrvDocumentPropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam)
{
	DUMPMSG("DrvDocumentPropertySheets");
    return vpui->DrvDocumentPropertySheets(pPSUIInfo,lParam);
}

BOOL  DrvPrinterEvent(LPWSTR  pPrinterName, INT  DriverEvent, DWORD  Flags, LPARAM  lParam)
{
	DUMPMSG("");
    return vpui->DrvPrinterEvent(pPrinterName,DriverEvent,Flags,lParam);
}

LONG  DrvDevicePropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam)
{
	DUMPMSG("DrvDevicePropertySheets");
    return vpui->DrvDevicePropertySheets(pPSUIInfo,lParam);
}

BOOL  DevQueryPrintEx(PDEVQUERYPRINT_INFO  pDQPInfo)
{
	DUMPMSG("DevQueryPrintEx");
    return vpui->DevQueryPrintEx(pDQPInfo);
}

BOOL  DrvConvertDevMode(LPTSTR  pPrinterName, PDEVMODE  pdmIn, PDEVMODE  pdmOut, PLONG  pcbNeeded,DWORD  fMode)
{
	DUMPMSG("DrvConvertDevMode");
    return vpui->DrvConvertDevMode(pPrinterName, pdmIn, pdmOut,pcbNeeded,fMode);
}

DWORD DrvSplDeviceCaps(
                                                HANDLE  hPrinter,
    __in                                        LPWSTR  pwDeviceName,
                                                WORD    DeviceCap,
    __out_ecount_opt(cchBuf) __typefix(TCHAR*)  VOID    *pvOutput,
                                                DWORD   cchBuf,
    __in_opt                                    DEVMODE *pDM
    )
{
	DUMPMSG("SplDeviceCap");
    switch (DeviceCap) {

    case DC_PAPERNAMES:
    case DC_MEDIAREADY:

        if (pvOutput) {

            if (cchBuf >= CCHPAPERNAME) {

                DeviceCap            = (DeviceCap == DC_PAPERNAMES) ?
                                                            DC_SPL_PAPERNAMES :
                                                            DC_SPL_MEDIAREADY;
                *((LPDWORD)pvOutput) = (DWORD)(cchBuf / CCHPAPERNAME);

                

            } else {

                return(GDI_ERROR);
            }
        }

        return(vpui->PMDrvDeviceCapabilities(hPrinter,
                                     pwDeviceName,
                                     DeviceCap,
                                     pvOutput,
                                     pDM));
        break;

    default:

        return(GDI_ERROR);
    }
}


