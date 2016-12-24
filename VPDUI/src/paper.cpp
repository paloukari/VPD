

#include "precomp.h"
#include <stdlib.h> 
#include <malloc.h>

#pragma hdrstop
extern size_t maxsize;


#include "vphelper.h"
#include <winspool.h>

PDEVMODE GetRealPrinterDevmode(HANDLE hRPrinter, WCHAR *RealDriverName, VDEVMODE *pCurDevmode)
{
     LONG sz = DocumentProperties(0,hRPrinter , RealDriverName,0,0,0);
     PDEVMODE pdmOutput = (PDEVMODE)malloc(sz);
     DocumentProperties(0,hRPrinter , RealDriverName,pdmOutput,0,DM_OUT_BUFFER);
     WORD dmDriverExtra =  pdmOutput->dmDriverExtra;
     memcpy(pdmOutput , pCurDevmode , sizeof(DEVMODEW));
     pdmOutput->dmDriverExtra = dmDriverExtra;
     DocumentProperties(0,hRPrinter , RealDriverName,pdmOutput,pdmOutput,
             DM_IN_BUFFER | DM_OUT_BUFFER);
     return pdmOutput;
}
void PrintToPaper(PPDEV pPDev)
{

     DOCINFO di;
     LPBYTE pDevmode;
     pDevmode = NULL;
     VDEVMODE *pCurDevmode = pPDev->pCurDevmode;
	 WCHAR *RealDriverName = pCurDevmode->pdm.PrinterName;

     HANDLE hRPrinter;   
     OpenPrinter(RealDriverName, &hRPrinter, NULL);
     PDEVMODE pdmOutput = GetRealPrinterDevmode(hRPrinter,RealDriverName,pCurDevmode);
     HDC hRealDC = CreateDC(L"WINSPOOL", RealDriverName, NULL, pdmOutput);
     free(pdmOutput);
     HDC hWorkingDC = hRealDC;
     memset(&di , 0 , sizeof(DOCINFO));
     di.lpszDocName = pPDev->pDocument;
     di.lpszDatatype = L"raw";
     if(StartDoc(hWorkingDC , &di) > 0)
     {

          for(int i = 1 ; i <= pPDev->Pages ; i++)
          {
               pDevmode = NULL;
               TCHAR MetaFileList[MAX_PATH];
               GetTempFile(NULL ,L"Meta" , MetaFileList);
               GetMetaFileFromSpoolFile(pPDev->pSpoolFileName ,i,MetaFileList,
                       pPDev, &pDevmode);
               if(pDevmode != NULL)
               {
                    PDEVMODE pdmOutput = GetRealPrinterDevmode(hRPrinter,RealDriverName,
                            (VDEVMODE *)pDevmode);
                    hWorkingDC = ResetDC(hWorkingDC , (DEVMODE *)pdmOutput);
                    free(pDevmode);
                    free(pdmOutput);
               }
               StartPage(hWorkingDC);
               HENHMETAFILE hemf = GetEnhMetaFile(
                       MetaFileList   // file name
                       );
               RECT rect;
               rect.left = rect.top = 0;
               rect.bottom = GetDeviceCaps(hWorkingDC , VERTRES);
               rect.right = GetDeviceCaps(hWorkingDC , HORZRES);
               PlayEnhMetaFile( hWorkingDC, hemf, &rect );
               EndPage(hWorkingDC);

               DeleteEnhMetaFile(hemf);
               DeleteFile(MetaFileList);
          }

     }
     EndDoc(hWorkingDC);
     ClosePrinter(hRPrinter);
     DeleteDC(hWorkingDC);
}

