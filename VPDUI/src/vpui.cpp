
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


//-----------------------------------------------------------------------------
LONG  VPDriver::DrvDocumentProperties(HWND hwnd, HANDLE hPrinter, PTSTR lpszDeviceName,
        PDEVMODE pdmOutput,PDEVMODE pdmInput, DWORD fMode,BOOL fromApp)
{     
     TCHAR DriverName[256];	 
	 TCHAR TargetDriverName[256];	 
     HANDLE hRPrinter = NULL;
     LPBYTE pBuffer = NULL;     
	 PDEVMODE pdm = NULL;

	 wcscpy_s(DriverName, wcsnlen(lpszDeviceName, 255)+1, lpszDeviceName);
     /*
      * If there is an incoming devmode, then take the DriverName from the
      * the private  part
      * else wherever it is stored.
      */

     if(pdmInput && (fMode & DM_IN_BUFFER) && pdmInput->dmSize == sizeof(DEVMODEW)
             && pdmInput->dmDriverExtra == sizeof(VPDEVMODE))
     {		 
		 VPDEVMODE *PrivateDevmode = (VPDEVMODE *)&(((VDEVMODE *)pdmInput)->pdm);		 
         wcscpy_s(TargetDriverName, wcsnlen(PrivateDevmode->PrinterName, maxsize)+1, PrivateDevmode->PrinterName);         
     }
     else
     {      
		 if((pdm = GetPrinterSettings(lpszDeviceName, (LPBYTE)pBuffer)) == NULL)
		 {
			 ClosePrinter(GetPrinterInfo(&pBuffer , lpszDeviceName));
             if(pBuffer)
				pdm = ((PRINTER_INFO_2 *)pBuffer)->pDevMode;
		 }		 
          

		if(pdm && pdm->dmDriverExtra == sizeof(VPDEVMODE))
		{
			VPDEVMODE *PrivateDevmode = &((VDEVMODE *)pdm)->pdm;			
			wcscpy_s(TargetDriverName , wcsnlen(PrivateDevmode->PrinterName, maxsize)+1, PrivateDevmode->PrinterName);
			OutputDebugString(PrivateDevmode->PrinterName);			
		}
		else
		{		//installation			
			GetTargetDriverName(hPrinter,TargetDriverName);
		}	
     }
     	 
	 OpenPrinter(TargetDriverName,&hRPrinter,NULL);     

     if(hRPrinter == NULL)
          return 0;   // fail gracefully
     
     PDEVMODE  pdmInputTargetDriver = NULL;
     PDEVMODE  pdmOutputTargetDriver = NULL;

	 GetDriverPDEVMODE(pdmInputTargetDriver, hRPrinter, TargetDriverName);
     WORD dmDriverExtra = pdmInputTargetDriver->dmDriverExtra;

     if(pdmInput && (fMode & DM_IN_BUFFER))
     {
          memcpy(pdmInputTargetDriver,pdmInput,sizeof(DEVMODEW));
          pdmInputTargetDriver->dmDriverExtra = (WORD)dmDriverExtra;
		  CopyDeviceName(pdmInputTargetDriver->dmDeviceName, TargetDriverName);		  		  
     }
     else if(pdm == NULL) //installation
     {
          LPBYTE pBuffer;
          ClosePrinter(GetPrinterInfo(&pBuffer , lpszDeviceName));
		  if(((PRINTER_INFO_2 *)pBuffer)->pDevMode != NULL)
		  {
			  //if we get the pBuffer == null keep the default pdmInputTargetDriver, it will work
			  memcpy(pdmInputTargetDriver,((PRINTER_INFO_2 *)pBuffer)->pDevMode,sizeof(DEVMODEW));
			  pdmInputTargetDriver->dmDriverExtra = (WORD)dmDriverExtra;
			  CopyDeviceName(pdmInputTargetDriver->dmDeviceName, TargetDriverName);		  
			  free(pBuffer);
		  }
     }

     if(pdmOutput)
     {
          pdmOutputTargetDriver = (PDEVMODE)malloc(GetDriverPDEVMODESize(hRPrinter, TargetDriverName));
     }

     LONG ret = DocumentProperties(0,hRPrinter,
             TargetDriverName,
             pdmOutputTargetDriver,pdmInputTargetDriver,fMode & (~DM_IN_PROMPT));

     if(((fMode & DM_OUT_BUFFER) || (fMode & DM_UPDATE)) && pdmOutput)
     {
          memset(pdmOutput , 0 , sizeof(VDEVMODE));
          memcpy(pdmOutput,pdmOutputTargetDriver,sizeof(DEVMODEW));
		  
		  CopyDeviceName(pdmOutput->dmDeviceName, lpszDeviceName);		  

          pdmOutput->dmDriverExtra = sizeof(VPDEVMODE);
          //fill in the VPDEVMODE we zeroed 
          VPDEVMODE *PrivateDevmode = &(((VDEVMODE *)pdmOutput)->pdm);
         
		  wcscpy_s(PrivateDevmode->PrinterName , wcsnlen(TargetDriverName, maxsize)+1, TargetDriverName);
          
     }               
	 if(pBuffer)
		 free(pBuffer);
     ClosePrinter(hRPrinter);
     if(pdmInputTargetDriver)
          free(pdmInputTargetDriver);
     if(pdmOutputTargetDriver)
          free(pdmOutputTargetDriver);
     return ret;
}

//-----------------------------------------------------------------------------
BOOL  VPDriver::DevQueryPrintEx(PDEVQUERYPRINT_INFO  pDQPInfo)
{
     return TRUE;
}

//-----------------------------------------------------------------------------
LONG  VPDriver::DrvDevicePropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam)
{

     DUMPMSG("DrvDevicePropertySheets");
     PDEVICEPROPERTYHEADER pDPHdr;
     if ((!pPSUIInfo) || (!(pDPHdr = (PDEVICEPROPERTYHEADER)pPSUIInfo->lParamInit))) {

          SetLastError(ERROR_INVALID_DATA);
          return(ERR_CPSUI_GETLASTERROR);
     }

     switch (pPSUIInfo->Reason)
     {
     case PROPSHEETUI_REASON_INIT:
         {


              DUMPMSG("DrvDevicePropertySheets Init ");
              PROPSHEETPAGE *psp = (PROPSHEETPAGE *)malloc(sizeof(PROPSHEETPAGE));
              memset(psp ,0, sizeof(PROPSHEETPAGE)); 
              
			  FillInPropertyPage( psp, IDD_DEVICEDIALOG, TEXT("VPD Settings"), (DLGPROC)DevicePropertiesDialog, (LPARAM)pDPHdr->hPrinter);
              
			  pPSUIInfo->UserData = NULL;

              if (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                          CPSFUNC_ADD_PROPSHEETPAGE,
                          (LPARAM)psp,
                          (LPARAM)0))
                   pPSUIInfo->Result = CPSUI_CANCEL;
         }
         return 1;

     case PROPSHEETUI_REASON_DESTROY:
         {
              HANDLE hPrinter =  pDPHdr->hPrinter;
              DWORD dwNeeded;
              GetPrinter( hPrinter, 2, NULL, 0, &dwNeeded );
              LPBYTE pi2 = (LPBYTE)malloc( dwNeeded );
              GetPrinter( hPrinter, 2, (LPBYTE)pi2, dwNeeded, &dwNeeded );              
              RemovePrinterSharing(((PRINTER_INFO_2 *)pi2)->pPrinterName,NULL,FALSE);
              free(pi2);
              return 1;

         }
     }

     return 1;
}

//-----------------------------------------------------------------------------
LONG  VPDriver::DrvDocumentPropertySheets(PPROPSHEETUI_INFO  pPSUIInfo, LPARAM  lParam)
{
     DUMPMSG("DrvDocumentPropertySheets");
     /*
      * Info without a dialog box
      */
     if (pPSUIInfo == NULL)
     {
          PDOCUMENTPROPERTYHEADER pDPH;
          pDPH = (PDOCUMENTPROPERTYHEADER)lParam;
          if (pDPH == NULL)
          {
               return -1;
          }
          /* return the devmode size */
          if (pDPH->fMode == 0 || pDPH->pdmOut == NULL)
          {
               pDPH->cbOut = sizeof(VDEVMODE);
               return pDPH->cbOut;
          }
          /* call the master function, DrvDocumentProperties for other processing */
          if (pDPH->fMode)
          {
               LONG pcbNeeded = pDPH->cbOut;
               if(DrvDocumentProperties(NULL, pDPH->hPrinter, pDPH->pszPrinterName, 
                       pDPH->pdmOut, pDPH->pdmIn, pDPH->fMode,TRUE) == 0)
                    return 0;
          }
          return 1;
     }

     switch (pPSUIInfo->Reason)
     {
     case PROPSHEETUI_REASON_INIT:
         {
              /*
               * Initialize the Property sheet
               */
              TCHAR TargetDriverName1[256];
              PROPSHEETPAGE *psp = (PROPSHEETPAGE *)malloc(sizeof(PROPSHEETPAGE));
              PDOCUMENTPROPERTYHEADER pDPH = (PDOCUMENTPROPERTYHEADER)lParam;
              TCHAR *TargetDriverName;
              LPBYTE pBuffer = NULL;

              PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };
              HANDLE hPrinter;
              OpenPrinter(pDPH->pszPrinterName,&hPrinter,&defaults);
              DWORD cbNeeded = 0;   
              BOOL Fail = FALSE;
              PDEVMODE pdm = NULL;
              if(GetPrinterData(hPrinter,L"PrinterSettings",NULL,
                          (LPBYTE)pBuffer,0,&cbNeeded) == ERROR_MORE_DATA)
              {
                   pBuffer = (LPBYTE)malloc(cbNeeded);

                   DWORD dwret = GetPrinterData(hPrinter,L"PrinterSettings",NULL,
                           (LPBYTE)pBuffer,cbNeeded,&cbNeeded);
                   if(dwret == ERROR_SUCCESS)
                        pdm = (PDEVMODE)pBuffer;
                   else
                        Fail = TRUE;
              }
              else
                   Fail = TRUE;

              if(Fail == TRUE)
              {
                   free(pBuffer);
                   ClosePrinter(hPrinter);
                   hPrinter = GetPrinterInfo(&pBuffer , pDPH->pszPrinterName);
                   if(pBuffer)
                        pdm = ((PRINTER_INFO_2 *)pBuffer)->pDevMode;
              }
			  TargetDriverName = (TCHAR *)((VDEVMODE*)pdm)->pdm.PrinterName;

              wcscpy_s(TargetDriverName1 , 1+wcsnlen(TargetDriverName, maxsize), TargetDriverName);
              ClosePrinter(hPrinter);
			  
              memset(psp ,0, sizeof(PROPSHEETPAGE)); 
              /* Prepare the PrinterSettings which will be used by the PropetySheet Dialog */
              VPrinterSettings *ps = (VPrinterSettings *)malloc(sizeof(VPrinterSettings));
              memset(ps, 0 , sizeof(VPrinterSettings));
              /* Intialize the PrinterSettings ValidDevMode with the default and then later 
               * with the inDevmode and then ValidateDevMode
               */
              if(pDPH->fMode & DM_IN_BUFFER)
                   ps->inDevmode = (VDEVMODE *)(pDPH->pdmIn);

              ps->ValidDevMode = (VDEVMODE *)malloc(sizeof(VDEVMODE));
              if(Fail  == TRUE)
              {
                   OutputDebugString(L"failure in propertysheet");
                   memcpy(ps->ValidDevMode , ((PRINTER_INFO_2 *)pBuffer)->pDevMode , 
                           sizeof(VDEVMODE));
              }
              else
                   memcpy(ps->ValidDevMode , pBuffer , sizeof(VDEVMODE));
              if(ps->inDevmode)
                   ValidateDevMode(ps->ValidDevMode , ps->inDevmode);
              free(pBuffer);

              /* Hold the pointer to outDevmode to be used in _SET_RESULT */
              if(pDPH->fMode & DM_OUT_BUFFER)
                   ps->outDevmode = (VDEVMODE *)(pDPH->pdmOut);

              /* Fill the PROPSHEETPAGE */
              FillInPropertyPage( psp, IDD_PRINTDIALOG, TEXT("Virtual Printer Settings"), (DLGPROC)DocumentProperty,(LPARAM)ps);
              wcscpy_s(PrinterName , 1+wcsnlen(pDPH->pszPrinterName, maxsize), pDPH->pszPrinterName);
              /* This so that WM_INITDIALOG gets as lparam the PrinterSettings */
              pPSUIInfo->UserData = (ULONG_PTR)ps;
              LONG_PTR result;
              /* Fill up the other members */
              ps->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
              ps->hComPropSheet = pPSUIInfo->hComPropSheet;
              ps->fMode = pDPH->fMode;
              ps->hPrinter = pDPH->hPrinter;
              
              if(pDPH->pszPrinterName)
              {
                   ps->pszPrinterName = (LPTSTR)malloc(sizeof(TCHAR) * (wcslen(pDPH->pszPrinterName)+ 1));
                   wcscpy_s(ps->pszPrinterName , 1+wcsnlen(pDPH->pszPrinterName, maxsize), pDPH->pszPrinterName);
              }
              if (result = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                          CPSFUNC_ADD_PROPSHEETPAGE,
                          (LPARAM)psp, 
                          (LPARAM)0))
              {
                   ps->handle = (HPROPSHEETPAGE)result;
                   pPSUIInfo->Result = CPSUI_CANCEL;
              }
              free(psp);

         }
         return 1;

     case PROPSHEETUI_REASON_SET_RESULT:
         {
              PSETRESULT_INFO     pSRInfo;

              pSRInfo = (PSETRESULT_INFO) lParam;

              if (pSRInfo->Result == CPSUI_OK)
              { 
                   /* User pressed ok, modify ValidDevmode  and send it to DrvDocumentProperties */
                   PDOCUMENTPROPERTYHEADER pDPH = (PDOCUMENTPROPERTYHEADER)lParam;
                   VPrinterSettings *ps = (VPrinterSettings *)(pPSUIInfo->UserData);
                   PDEVMODE pdmValid = (PDEVMODE)(ps->ValidDevMode);
                   
                   DrvDocumentProperties(NULL, ps->hPrinter, ps->pszPrinterName, 
                           (DEVMODE *)(ps->outDevmode), (DEVMODE *)(ps->ValidDevMode), (ps->fMode & ~DM_PROMPT));
                   /* 
                    * Set the private part of the outDevmode with the corresponding members from
                    * PrinterSettings
                    */
                   VDEVMODE *pdmOutput = (VDEVMODE*)(ps->outDevmode);
                   VPDEVMODE *PrivateDevmode = &(pdmOutput->pdm);
                   
                   wcscpy_s(PrivateDevmode->PrinterName ,1+wcsnlen(ps->PrinterName, maxsize), ps->PrinterName );
                   
				   DrvDocumentProperties(NULL, ps->hPrinter, ps->pszPrinterName, 
                           NULL, (DEVMODE *)(ps->outDevmode), DM_IN_BUFFER,TRUE);
                   
				   if(bIsExplorer)
                   {
                        HANDLE hPrinter;
                        PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };

                        OpenPrinter(
                                ps->pszPrinterName,         // printer or server name
                                &hPrinter,          // printer or server handle
                                &defaults   // printer defaults
                                );
                        DWORD dwret = SetPrinterData(
                                hPrinter,    // handle to printer or print server
                                L"PrinterSettings",  // data to set
                                REG_BINARY,         // data type
                                (LPBYTE)(ps->outDevmode),       // configuration data buffer
                                sizeof(VDEVMODE)
                                );
                        SetTargetDriverName(ps->pszPrinterName , PrivateDevmode->PrinterName);
                        ClosePrinter(hPrinter);
                   }
                   wcscpy_s(TargetPrinterName , 1+wcsnlen(ps->PrinterName, maxsize), ps->PrinterName);
              }
         }
         break;
     case PROPSHEETUI_REASON_DESTROY:
         {
              /* Cleanup */
              VPrinterSettings *ps = (VPrinterSettings *)(pPSUIInfo->UserData);
              free(ps->pszPrinterName);
              free(ps->ValidDevMode);
              free((void *)(ps));
              pPSUIInfo->UserData = NULL;
         }
         return 1;
     }

     return 1;
}

//-----------------------------------------------------------------------------
BOOL  VPDriver::DrvPrinterEvent(LPWSTR  pPrinterName, INT  DriverEvent, DWORD  Flags, LPARAM  lParam)
{
     if(DriverEvent == PRINTER_EVENT_INITIALIZE)
     {
          TCHAR TargetDriverName[256];
          memset(TargetDriverName , 0 , sizeof(TCHAR) * 256);
          GetTargetFromRegistry(TargetDriverName);
          SetTargetDriverName(pPrinterName , TargetDriverName);
     }
     return TRUE;
}

//-----------------------------------------------------------------------------
DWORD  VPDriver::PMDrvDeviceCapabilities(HANDLE  hPrinter,PWSTR  pDeviceName, WORD  iDevCap,
        VOID  *pvOutput,DEVMODE  *pDevMode)

{
     TCHAR TargetDriverName[256];
     LPBYTE pBuffer;   
     /* Target driver name has  3 tries
      *  First devmode,second if no incoming devmode,then the PrinterName
      *  Third from the default Devmode.
      */
     if(pDevMode)
		 wcscpy_s(TargetDriverName , 1+wcsnlen(((VDEVMODE*)pDevMode)->pdm.PrinterName, maxsize), ((VDEVMODE*)pDevMode)->pdm.PrinterName);
     else if(wcscmp(TargetPrinterName , L""))
          wcscpy_s(TargetDriverName , 1+wcsnlen(TargetPrinterName, maxsize), TargetPrinterName);
     else
     {
          LPBYTE pBuffer = NULL;
          ClosePrinter(GetPrinterInfo(&pBuffer ,pDeviceName));
          PDEVMODE pdm = NULL;
          if(pBuffer)
               pdm =((PRINTER_INFO_2 *)pBuffer)->pDevMode;
          if(pdm && pdm->dmDriverExtra == sizeof(VPDEVMODE))
          {
               wcscpy_s(TargetDriverName ,1+wcsnlen(((VDEVMODE*)pdm)->pdm.PrinterName, maxsize), ((VDEVMODE*)pdm)->pdm.PrinterName);
          }
          else
               GetTargetDriverName(hPrinter , TargetDriverName);
          free(pBuffer);
     }
     LPBYTE pBuffer3;
     HANDLE hRPrinter = GetPrinterInfo(&pBuffer3 , TargetDriverName);
     if(hRPrinter == NULL)
     {
          GetTargetDriverName(hPrinter , TargetDriverName);
          hRPrinter = GetPrinterInfo(&pBuffer , TargetDriverName);
     }
     else pBuffer = pBuffer3;

     /* If there is an incoming devmode, 
        1) Get the real driver's devmode size
        2) Get the real driver's devmode
        3) copy our public part to real driver's devmode
      */
     PDEVMODE  pdmInput = NULL;
     LPBYTE pBuffer1;
     ClosePrinter(GetPrinterInfo(&pBuffer1 , pDeviceName));
     LONG sz = DocumentProperties(0,hRPrinter , TargetDriverName,0,0,0);
     pdmInput = (PDEVMODE)malloc(sz);
     DocumentProperties(0,hRPrinter , TargetDriverName,pdmInput,0,DM_OUT_BUFFER);
     WORD dmDriverExtra = pdmInput->dmDriverExtra;
     if(pDevMode)
     {
          memcpy(pdmInput,pDevMode,sizeof(DEVMODEW));
          pdmInput->dmDriverExtra = dmDriverExtra;
     }
     else
     {
          memcpy(pdmInput,((PRINTER_INFO_2 *)pBuffer1)->pDevMode,sizeof(DEVMODEW));
          pdmInput->dmDriverExtra = dmDriverExtra;
          free(pBuffer1);
     }

     ClosePrinter(hRPrinter);
     /* The real driver should fix up the devmode and then do the rest */
     DWORD ret =  DeviceCapabilities( TargetDriverName, ((PRINTER_INFO_2 *)pBuffer)->pPortName,
             iDevCap, (LPTSTR)pvOutput, pdmInput);
     free(pdmInput);
     free(pBuffer);
     return ret;

}

//-----------------------------------------------------------------------------
BOOL VPDriver::DrvConvertDevMode(LPTSTR  pPrinterName, PDEVMODE  pdmIn, PDEVMODE  pdmOut, 
        PLONG  pcbNeeded,DWORD  fMode)
{
     HANDLE hPrinter;
     {
          wcscpy_s(PrinterName , 1+wcsnlen(pPrinterName, maxsize),  pPrinterName);
          LONG sz = sizeof(VDEVMODE);
          if(!pdmOut || sz > *pcbNeeded)
          {
               SetLastError(ERROR_INSUFFICIENT_BUFFER);
               return FALSE;
          }
          OpenPrinter(pPrinterName, &hPrinter, NULL);
          if(hPrinter)
          {
               DrvDocumentProperties(NULL, hPrinter, pPrinterName, pdmOut, NULL, DM_OUT_BUFFER);
               ClosePrinter(hPrinter);
          }
     }

     return TRUE; 
}


//-----------------------------------------------------------------------------
VPDriver::VPDriver(BOOL bIsExp)
{ 
	bIsExplorer = bIsExp; 
	wcscpy_s(TargetPrinterName , wcsnlen(L"", maxsize)+1, L"");	
}

//-----------------------------------------------------------------------------
void VPDriver::FixUpDevmode(IN HANDLE hPrinter , IN DOCEVENT_CREATEDCPRE *pbIn, IN OUT PULONG pbOut)
{
     LPBYTE pPrinterInfo;
     DWORD cbNeeded;
     GetPrinter(hPrinter,2,NULL ,0,&cbNeeded);
     pPrinterInfo = (LPBYTE)malloc(cbNeeded);
     GetPrinter(hPrinter,2,pPrinterInfo,cbNeeded,&cbNeeded);
     size_t sz;
     VDEVMODE *pdm = 
         (VDEVMODE *)malloc(sz = (sizeof(VDEVMODE)));
	 PDEVMODE pInDevmode = (PDEVMODE)pbIn->pdm;
     if(pInDevmode->dmSize == sizeof(DEVMODEW) 
             && pInDevmode->dmDriverExtra  == sizeof(VPDEVMODE))
          memcpy((void *)pdm,(void *)pInDevmode , sz);
     else
     {
          if(!IsSpooler())
               DrvDocumentProperties(0,hPrinter , 
                       ((PRINTER_INFO_2 *)pPrinterInfo)->pPrinterName
                       ,(PDEVMODE)pdm,(PDEVMODE)pbIn->pdm,
                       DM_IN_BUFFER | DM_OUT_BUFFER);
     }
     *pbOut = (ULONG)pdm;
     CreateGDIInfo(hPrinter,pdm);
     free(pPrinterInfo);
}

//-----------------------------------------------------------------------------
INT VPDriver::DrvDocumentEventImpl(
        HANDLE  hPrinter,
        HDC  hdc,
        int  iEsc,
        ULONG  cbIn,
        PULONG  pbIn,
        ULONG  cbOut,
        PULONG  pbOut
        )
{
     DUMPMSG("DrvDocumentEventImpl");
     static DWORD Pages = 0;
     static BOOL *pResetDC = NULL;
     static DWORD dwJobId;
     switch(iEsc)
     {	  
     case DOCUMENTEVENT_RESETDCPOST: 
     case DOCUMENTEVENT_CREATEDCPOST:
         {
              free((void *)pbIn[0]);
         }
         break;
     case DOCUMENTEVENT_RESETDCPRE:
         {
              DUMPMSG("DrvDocumentEventreset");
              if(!pResetDC)
                   pResetDC = (BOOL *)malloc(sizeof(BOOL) * (Pages + 1));    
              pResetDC[Pages] = TRUE;
         }
     case DOCUMENTEVENT_CREATEDCPRE:
         {
              CreateWin2kcompatibleSplFile(hPrinter);

              //if(iEsc == DOCUMENTEVENT_CREATEDCPRE)
                   FixUpDevmode(hPrinter , ((DOCEVENT_CREATEDCPRE*)pbIn), pbOut);
              //else 
                   //FixUpDevmode(hPrinter , (PDEVMODE)pbIn[0], pbOut);

         }
         break;
     case DOCUMENTEVENT_STARTPAGE:
         DUMPMSG("DrvDocumentEventstartpage");
         if(!pResetDC)
         {
              pResetDC = (BOOL *)malloc(sizeof(BOOL));    
              pResetDC[0] =FALSE;
         }
         Pages++;
         break;
     case DOCUMENTEVENT_ENDPAGE:
         {
              DUMPMSG("DrvDocumentEventendpage");
              BOOL *pTemp = (BOOL *)malloc(sizeof(BOOL) * (Pages + 1));    
              if(pResetDC)
              {
                   memcpy(pTemp , pResetDC , sizeof(BOOL) * Pages );
                   free(pResetDC);
              }
              pResetDC = pTemp;
              pResetDC[Pages] = FALSE;
         }
         break;
     case DOCUMENTEVENT_ENDDOC:
         {
              /* You need administrator access to the spool directory for the below to work */
              if(IsSpooler())
              {
                   DWORD dwSize = sizeof(DEVDATA) - sizeof(VDEVMODE *) - sizeof(LPTSTR);

                   DEVDATA PDEV;
                   ZeroMemory(&PDEV , sizeof(DEVDATA));
                   ExtEscape(hdc,PDEV_ESCAPE, 0 , NULL , dwSize, (LPSTR)&PDEV); 
                   PDEV.pResetDC = pResetDC;
                   TCHAR SpoolFileName[MAX_PATH];
                   GetSpoolFileName(PDEV.dwJobId , SpoolFileName , hPrinter);
                   PDEV.hPDriver = hPrinter;
                   TCHAR TempSpoolFileName[MAX_PATH];
                   GetTempFile(NULL ,L"PM" , TempSpoolFileName);
                   CopyFile(SpoolFileName , TempSpoolFileName, FALSE);
                   PDEV.pSpoolFileName = TempSpoolFileName;
                   DWORD cbNeeded;
                   GetJob( PDEV.hPDriver, PDEV.dwJobId, 2, 0, 0, &cbNeeded);
                   LPBYTE pJobInfo = (LPBYTE)malloc(cbNeeded);
                   GetJob( PDEV.hPDriver, PDEV.dwJobId, 2, pJobInfo, cbNeeded, &cbNeeded);
                   PDEV.pCurDevmode = (VDEVMODE *)(((JOB_INFO_2 *)pJobInfo)->pDevMode);
                   PDEV.pDocument = (((JOB_INFO_2 *)pJobInfo)->pDocument);
				   PrintToPaper(&PDEV);
                   /*DialogBoxParam( (HINSTANCE)GetHMODULE(), MAKEINTRESOURCE(IDD_SAVEDIALOG),
                           GetDesktopWindow(), (DLGPROC)PMDialog, (LPARAM)(&PDEV));*/
                   dwJobId = PDEV.dwJobId;
                   free(pJobInfo);
                   DeleteFile(TempSpoolFileName);
              }
              Pages  = 0;
              if(pResetDC)
              {
                   free(pResetDC);
                   pResetDC = NULL;
              }

         }
         break;
     case DOCUMENTEVENT_DELETEDC:
         break;
     case DOCUMENTEVENT_STARTDOC:
         break;
     case DOCUMENTEVENT_ENDDOCPOST:
         {
              if(IsSpooler())
              {
                   PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS};
                   HANDLE hGPrinter;

                   TCHAR PrintMirrorName[256];

                   GetPrintMirrorName(PrintMirrorName, hPrinter);
                   OpenPrinter(PrintMirrorName, &hGPrinter, NULL);
                   LPVOID lpv = malloc(sizeof(HANDLE) + sizeof(DWORD));
                   *((HANDLE *)lpv) = hGPrinter;
                   *(DWORD *)((LPBYTE)lpv + sizeof(HANDLE)) = dwJobId;
                   HANDLE hThread =  CreateThread( NULL,   0, ThreadFunc, lpv, 0, NULL);     
              }
              //_CrtDumpMemoryLeaks();
         }
         break;
     }

     return DOCUMENTEVENT_SUCCESS;
}
