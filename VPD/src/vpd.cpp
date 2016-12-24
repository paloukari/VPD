
#include "precomp.h"
#include   <stdlib.h>
#include   <windows.h>
#include   <winspool.h>
#include   <stdio.h>
#include   <wingdi.h>
#include   <winddi.h>
#include "vpd.h"


#define COPYMEMORY(a,b,c) CopyMemory(a,b,c)
#define ZEROMEMORY(a,b) ZeroMemory(a,b)
#define FREE(X)  free(X)
#define MALLOC(x) malloc(x)
#define GETPRINTERDATA GetPrinterData
#define GETPRINTERDRIVER GetPrinterDriver




HANDLE hModDll;

typedef struct _DEVDATA{
    HPALETTE hPalDefault ;
    HDEV hEngineDevice;
    HANDLE hPDriver;
    HSURF  hSurf;
    BOOL  binDocument;
    DWORD dwJobId;
    int Pages;
    GDIINFO DevCaps;
    DRVENABLEDATA pReal;
    DHPDEV dhRPdev; 
    HMODULE hRealDriver;
    int iDitherFormat; 
}DEVDATA,  *PDEVDATA;

size_t maxsize = 1024;
ULONG palette[256];


BOOL
FillDevInfo(
        PDEVDATA   pDevData,
        DEVINFO *pDevInfo,
        int numcolors,
        ULONG *palette,
        LOGFONT *lf
        
        )

{
     /* In case of any probs uncomment this
      */
     //
     // Start with a clean slate.
     //
     ZEROMEMORY(pDevInfo, sizeof(DEVINFO));

     pDevInfo->flGraphicsCaps = GCAPS_ALTERNATEFILL  |
         GCAPS_HORIZSTRIKE    |
         GCAPS_VERTSTRIKE     |
         GCAPS_VECTORFONT | GCAPS_HALFTONE | GCAPS_OPAQUERECT |
         GCAPS_BEZIERS |
         GCAPS_WINDINGFILL;
     pDevInfo->cxDither = 0;
     pDevInfo->cyDither = 0;
     switch(numcolors)
     {
     case 2:
         pDevInfo->iDitherFormat = BMF_1BPP;
         break;
     case 16:
         pDevInfo->iDitherFormat = BMF_4BPP;
         break;
     case 256:
         pDevInfo->iDitherFormat = BMF_8BPP;
         break;
     case 0:
         pDevInfo->iDitherFormat = BMF_24BPP;
         break;
     }
     wcscpy_s((PWSTR)pDevInfo->lfDefaultFont.lfFaceName, 1+wcsnlen(lf->lfFaceName, maxsize), lf->lfFaceName);
     pDevInfo->lfDefaultFont.lfEscapement = lf->lfEscapement;
     pDevInfo->lfDefaultFont.lfOrientation = lf->lfOrientation;
     pDevInfo->lfDefaultFont.lfHeight = lf->lfHeight;
     pDevInfo->lfDefaultFont.lfWidth = lf->lfWidth;
     pDevInfo->lfDefaultFont.lfWeight = lf->lfWeight;
     pDevInfo->lfDefaultFont.lfItalic = lf->lfItalic;
     pDevInfo->lfDefaultFont.lfUnderline = lf->lfUnderline;
     pDevInfo->lfDefaultFont.lfStrikeOut = lf->lfStrikeOut;
     pDevInfo->lfDefaultFont.lfPitchAndFamily = lf->lfPitchAndFamily;
     memcpy((void *)&(pDevInfo->lfAnsiVarFont), lf, sizeof(LOGFONT));
     memcpy((void *)&(pDevInfo->lfAnsiFixFont), lf, sizeof(LOGFONT));
     
     if(numcolors)
         pDevInfo->hpalDefault = EngCreatePalette(PAL_INDEXED, numcolors,palette, 0, 0, 0);
     else
		 pDevInfo->hpalDefault = EngCreatePalette(PAL_BGR, 0, 0, 0, 0, 0);

     pDevData->iDitherFormat = pDevInfo->iDitherFormat ;
     pDevData->hPalDefault = pDevInfo->hpalDefault;

     return(TRUE);
}




/***************************Export   Functions   by  Dll   ************************/
BOOL  CALLBACK
DllMain(
        HANDLE      hModule,
        ULONG       ulReason,
        PCONTEXT    pContext
       )
{
     switch (ulReason)
     {
     case DLL_PROCESS_ATTACH:
         {
              hModDll = hModule;
              /* keep the dll loaded */
              TCHAR wName[MAX_PATH];
              if (GetModuleFileName((HINSTANCE)hModule, wName, MAX_PATH)) 
                   LoadLibrary(wName);
              break;
         }

     case DLL_PROCESS_DETACH:
         break;
     }

     return TRUE;
}



/*****I    have   seen    this   getting   called   at   the
  first  print for a  spooler   session   and never   after******************/
#ifdef USERMODE_DRIVER
extern "C" BOOL CALLBACK DrvQueryDriverInfo(DWORD dwMode, PVOID pBuffer, DWORD cbBuf, PDWORD pcbNeeded)
{
     DUMPMSG("DrvQueryDriverInfo");

     switch (dwMode)
     {
     case DRVQUERY_USERMODE:
         *pcbNeeded = sizeof(DWORD);

         if (pBuffer == NULL || cbBuf < sizeof(DWORD))
         {
              SetLastError(ERROR_INSUFFICIENT_BUFFER);
              return FALSE;
         }

         *((PDWORD) pBuffer) = TRUE;
         return TRUE;

     default:
         SetLastError(ERROR_INVALID_PARAMETER);
         return FALSE;
     }
}
#endif

BOOL CALLBACK DrvStartBanding( SURFOBJ *pso, POINTL *pptl )
{
     DUMPMSG("DrvStartBanding");
     return TRUE;
}

BOOL CALLBACK DrvNextBand( SURFOBJ *pso, POINTL *pptl )
{
     DUMPMSG("DrvNextBand");
     return TRUE;
}



/***************Document  Handling***********************************************/

extern "C" DHPDEV CALLBACK
DrvEnablePDEV(
        DEVMODEW *pdm,
        LPWSTR    pwszLogAddress,
        ULONG     cPat,
        HSURF    *phsurfPatterns,
        ULONG     cjCaps,
        ULONG    *pdevcaps,
        ULONG     cjDevInfo,
        DEVINFO  *pdi,
        HDEV      pwszDataFile,   // PWSTR
        LPWSTR    pwszDeviceName,
        HANDLE    hDriver)
{
	DUMPMSG("DrvEnablePdev");

     DHPDEV dhpdev = NULL;    
     PDEVDATA pDevData = (PDEVDATA)MALLOC(sizeof(DEVDATA));
     ZEROMEMORY(pDevData, sizeof(DEVDATA));
     pDevData->hPDriver = hDriver;
     pDevData->dwJobId = 9999; //REVISIT  here
     
     int  numcolors = 0;
     VDEVMODE *pDevMode = (VDEVMODE *)pdm;
     COPYMEMORY(pdevcaps , &(pDevMode->pdm.gi) , sizeof(GDIINFO));
     numcolors = pDevMode->pdm.numcolors;
     COPYMEMORY(palette , pDevMode->pdm.Palette , sizeof(ULONG) * 256);
     LOGFONT lf;
     COPYMEMORY(&lf , &(pDevMode->pdm.lf), sizeof(LOGFONT));

     COPYMEMORY(
             PVOID(&(pDevData->DevCaps)),(PVOID)pdevcaps, sizeof(GDIINFO));
     FillDevInfo(
             pDevData,
             pdi,numcolors,palette,&lf
             );
     pdi->flGraphicsCaps &= ~GCAPS_DONTJOURNAL;
     return((DHPDEV)pDevData);
}


extern "C" VOID CALLBACK
DrvCompletePDEV(DHPDEV dhpdev,HDEV hdev)
{
     DUMPMSG("DrvCompletePDEV");
     ((DEVDATA *)dhpdev)->hEngineDevice = hdev;
}


extern "C" VOID CALLBACK
DrvDisablePDEV(DHPDEV dhpdev)
{
     DUMPMSG("DisablePDev");
     EngDeletePalette(((DEVDATA *)dhpdev)->hPalDefault); 
     FREE((PDEVDATA)dhpdev);
     return;
}


extern "C" HSURF CALLBACK
DrvEnableSurface(DHPDEV dhpdev)
{

     HSURF hSurf = NULL;
     long  hooks;
     //----------------------------------------------------------------
     // Setup size of device
     //----------------------------------------------------------------
     SIZEL       deviceSize;

     DUMPMSG("DrvEnableSurface");

     deviceSize.cx =  ((DEVDATA *)dhpdev)->DevCaps.ulHorzRes;
     deviceSize.cy = ((DEVDATA *)dhpdev)->DevCaps.ulVertRes;


     hSurf = EngCreateDeviceSurface((DHSURF)dhpdev, deviceSize, ((DEVDATA *)dhpdev)->iDitherFormat);
     if(hSurf == NULL)
	 {
		DUMPMSG("Surface Failure");
		return hSurf;
	 }
     hooks= HOOK_BITBLT | HOOK_STRETCHBLT | HOOK_TEXTOUT | HOOK_STROKEPATH | HOOK_FILLPATH |  HOOK_COPYBITS | HOOK_STRETCHBLTROP| HOOK_STROKEANDFILLPATH;
     EngAssociateSurface( hSurf, ((DEVDATA *)dhpdev)->hEngineDevice, hooks );
     ((DEVDATA*)dhpdev)->hSurf = hSurf;
     return hSurf;
}


extern "C" VOID CALLBACK
DrvDisableSurface(DHPDEV dhpdev)
{
     /*  delete the bitmap now */
     DUMPMSG("DrvDisableSurface"); 
     EngDeleteSurface(((DEVDATA*)dhpdev)->hSurf);
     ((DEVDATA*)dhpdev)->hSurf = NULL;
     return;
}



/*
 * No real resource swapping here.
 * DrvRestPdev is visited in the following order
 * oldpdev = DrvEnablePdev(...)
 * DrvCompletepdev(oldpdev,...)
 * newpdev = DrvEnablepdev(...)
 * DrvResetpdev(old,new)
 * DrvDisablepdev(oldpdev)
 * we return  TRUE as we don't have much to do here
 * Plan to keep the devmode here or we could take from EMF
 */
extern "C" BOOL  CALLBACK DrvResetPDEV(DHPDEV dhpdevOld, DHPDEV dhpdevNew)
{
     DUMPMSG("DrvResetPDEV");
     PDEVDATA pDevDataNew = (PDEVDATA)dhpdevNew;
     PDEVDATA pDevDataOld = (PDEVDATA)dhpdevOld;

     pDevDataNew->dwJobId = pDevDataOld->dwJobId;
     pDevDataNew->Pages = pDevDataOld->Pages;
     pDevDataNew->binDocument = pDevDataOld->binDocument;
     return TRUE;
}
//  DrvDisableDriver    doesn't   seemed    to  be   called  anytime  in  2k.
//  Still  we  implement  and  keep.
extern "C" VOID CALLBACK DrvDisableDriver()
{
}

/***************Brush   Entries***********************************************/

extern "C" BOOL CALLBACK DrvRealizeBrush(BRUSHOBJ *pbo,SURFOBJ  *psoTarget,SURFOBJ  *psoPattern,SURFOBJ  *psoMask,XLATEOBJ *pxlo,ULONG    iHatch)
{
     return TRUE;
}



/****************Vector   Enties**************************************************/
extern "C" BOOL CALLBACK DrvStrokePath(SURFOBJ   *pso,PATHOBJ   *ppo,CLIPOBJ   *pco,XFORMOBJ  *pxo,BRUSHOBJ  *pbo,POINTL    *pptlBrushOrg,LINEATTRS *plineattrs,MIX        mix)
{

     DUMPMSG("StrokePath");
     return TRUE;
}


extern "C" BOOL CALLBACK DrvFillPath(SURFOBJ  *pso,PATHOBJ  *ppo,CLIPOBJ  *pco,BRUSHOBJ *pbo,POINTL   *pptlBrushOrg,MIX       mix,FLONG     flOptions)
{

     DUMPMSG("DrvFillPath");
     return TRUE;
}


extern "C" BOOL CALLBACK DrvStrokeAndFillPath(SURFOBJ   *pso,PATHOBJ   *ppo,CLIPOBJ   *pco,XFORMOBJ  *pxo,BRUSHOBJ  *pboStroke,LINEATTRS *plineattrs,BRUSHOBJ  *pboFill,POINTL    *pptlBrushOrg,MIX        mixFill,FLONG      flOptions)
{

     DUMPMSG("DrvStrokeAndFillPath");
     return TRUE;
}


extern "C" BOOL CALLBACK DrvPaint(SURFOBJ  *pso,CLIPOBJ  *pco,BRUSHOBJ *pbo,POINTL   *pptlBrushOrg,MIX       mix)
{

     DUMPMSG("DrvPaint");
     return TRUE;
}

extern "C" BOOL CALLBACK DrvLineTo(SURFOBJ   *pso,CLIPOBJ   *pco,BRUSHOBJ  *pbo,LONG       x1,LONG       y1,LONG       x2,LONG       y2,RECTL     *prclBounds,MIX        mix)
{
     DUMPMSG("LineTo");

     return TRUE;
}
/****************************Raster   Entries*************************************/

extern "C" HBITMAP CALLBACK DrvCreateDeviceBitmap(
        DHPDEV  dhpdev,
        SIZEL  sizl,
        ULONG  iFormat
        )
{
     DUMPMSG("DrvCreateDeviceBitmap");
     return 0;

}

extern "C" VOID CALLBACK DrvDeleteDeviceBitmap(
        DHSURF  dhsurf
        )
{
     DUMPMSG("DrvDeleteDeviceBitmap");
}
extern "C" BOOL CALLBACK DrvBitBlt(SURFOBJ  *psoTrg,SURFOBJ  *psoSrc,SURFOBJ  *psoMask,CLIPOBJ  *pco,XLATEOBJ *pxlo,RECTL    *prclTrg,POINTL   *pptlSrc,POINTL   *pptlMask,BRUSHOBJ *pbo,POINTL   *pptlBrush,ROP4      rop4)
{
     DUMPMSG("DrvBitBlt");

     return TRUE;
}


extern "C" BOOL CALLBACK DrvCopyBits(SURFOBJ  *psoDest,SURFOBJ  *psoSrc,CLIPOBJ  *pco,XLATEOBJ *pxlo,RECTL    *prclDest,POINTL   *pptlSrc)
{
     DUMPMSG("DrvCopyBits");

     return TRUE;
}



extern "C" BOOL CALLBACK DrvStretchBlt(SURFOBJ * psoDest, SURFOBJ * psoSrc,
        SURFOBJ * psoMask, CLIPOBJ * pco,
        XLATEOBJ * pxlo, COLORADJUSTMENT * pca,
        POINTL * pptlHTOrg, RECTL * prclDest,
        RECTL * prclSrc, POINTL * pptlMask,
        ULONG iMode)
{
     DUMPMSG("DrvStretchBlt");

     return TRUE;

}

extern "C" BOOL CALLBACK DrvStretchBltROP( SURFOBJ  *pTargetSurfObj, SURFOBJ   *pSourceSurfObj,
        SURFOBJ                 *pMaskSurfObj, CLIPOBJ   *pClipObj,
        XLATEOBJ                *pXlateObj, COLORADJUSTMENT    *pColorAdjustment,
        POINTL                  *pHalfToneBrushOriginPointl, RECTL     *pTargetRectl,
        RECTL                   *pSourceRectl, POINTL     *pMaskOffsetPointl,
        ULONG                   mode, BRUSHOBJ    *pBrushObj,
        ROP4                    rop4 //  some  places   this   also  referred  as  mix.
        )
{

     return TRUE;
}

/***********************TextOut  Entry***************************************/
extern "C" BOOL CALLBACK
DrvTextOut(SURFOBJ  *pso,
        STROBJ   *pstro,
        FONTOBJ  *pfo,
        CLIPOBJ  *pco,
        RECTL    *prclExtra,
        RECTL    *prclOpaque,
        BRUSHOBJ *pboFore,
        BRUSHOBJ *pboOpaque,
        POINTL   *pptlOrg,
        MIX       mix)
{
     DUMPMSG("DrvTextOut");     

     return TRUE;

}

/************************Escape    Entry*********************************/
#define PDEV_ESCAPE 0x303eb8efU  //this is for the job id.
extern "C" ULONG CALLBACK
DrvEscape(SURFOBJ *pso,
        ULONG    iEsc,
        ULONG    cjIn,
        PVOID    pvIn,
        ULONG    cjOut,
        PVOID    pvOut)
{
     ULONG uRetVal = 0;
     DUMPMSG("DrvEscape");
     struct DEVDATAUI{
          DWORD dwJobId;
          int Pages;
          TCHAR *pSpoolFileName;
          HANDLE hPDriver;
          BOOL *pResetDC;
     };
     switch(iEsc)
     {
          /* while playing back on the spooler we don't get jobid */
     case PDEV_ESCAPE: 
         {
              PDEVDATA pDevData = (DEVDATA *)pso->dhpdev;
              DEVDATAUI *pDevUI = (DEVDATAUI *)pvOut;
              pDevUI->dwJobId = pDevData->dwJobId;
              pDevUI->Pages = pDevData->Pages;
              return TRUE; 
         }
         break;
     }
     return uRetVal;
}


ULONG CALLBACK DrvDrawEscape(SURFOBJ *pso,ULONG    iEsc,CLIPOBJ *pco,RECTL   *prcl,ULONG    cjIn,PVOID    pvIn)
{
     return 0;
}



extern "C" BOOL CALLBACK DrvStartDoc(SURFOBJ *pso, LPWSTR pwszDocName, DWORD dwJobId)
{
     DUMPMSG("DrvStartDoc");

     //REVISIT for 9999, we swap anyway in ResetPdev for ResetDc
     if(((DEVDATA *)pso->dhpdev)->dwJobId == 9999)
     {
          ((DEVDATA *)(pso->dhpdev))->Pages = 0;
          ((DEVDATA *)pso->dhpdev)->dwJobId = dwJobId;
          ((DEVDATA *)pso->dhpdev)->binDocument = TRUE;
     }     
     return TRUE;
}

extern "C" BOOL  CALLBACK DrvEndDoc(SURFOBJ *pso, FLONG fl)
{
     DUMPMSG("DrvEndDoc");
     ((DEVDATA *)(pso->dhpdev))->dwJobId = 9999; 
     ((DEVDATA *)(pso->dhpdev))->Pages = 0; 
     return TRUE;

}

extern "C" BOOL  CALLBACK DrvStartPage(SURFOBJ *pso)
{
     DUMPMSG("DrvStartPage");
     return TRUE;
}

//Doesn't get called when we have a banding surface, we will never be so.
extern "C" BOOL  CALLBACK DrvSendPage(SURFOBJ *pso)
{
     DUMPMSG("DrvSendPage");
     ((DEVDATA *)(pso->dhpdev))->Pages++;
     return TRUE;
}

static const DRVFN DrvFuncTable[] = {

        //{  INDEX_DrvDisableDriver,       (PFN)DrvDisableDriver      },
        {  INDEX_DrvEnablePDEV,          (PFN)DrvEnablePDEV         },
        {  INDEX_DrvResetPDEV,           (PFN)DrvResetPDEV          },
        {  INDEX_DrvCompletePDEV,        (PFN)DrvCompletePDEV       },
        {  INDEX_DrvDisablePDEV,         (PFN)DrvDisablePDEV        },
        {  INDEX_DrvEnableSurface,       (PFN)DrvEnableSurface      },      
		{	INDEX_DrvStretchBltROP,		 (PFN)DrvStretchBltROP        },
		{	INDEX_DrvDisableSurface,     (PFN)DrvDisableSurface     },
        {  INDEX_DrvStrokePath,          (PFN)DrvStrokePath         },
        {  INDEX_DrvStrokeAndFillPath,   (PFN)DrvStrokeAndFillPath  },
        {  INDEX_DrvFillPath,            (PFN)DrvFillPath           },
        {  INDEX_DrvRealizeBrush,        (PFN)DrvRealizeBrush       },
        {  INDEX_DrvBitBlt,              (PFN)DrvBitBlt             },
        {  INDEX_DrvStretchBlt,          (PFN)DrvStretchBlt         },
        {  INDEX_DrvCopyBits,            (PFN)DrvCopyBits           },
        {  INDEX_DrvPaint,               (PFN)DrvPaint              },        
        {  INDEX_DrvTextOut,             (PFN)DrvTextOut            },
        {  INDEX_DrvSendPage,            (PFN)DrvSendPage           },
        {  INDEX_DrvStartPage,           (PFN)DrvStartPage          },
        {  INDEX_DrvStartDoc,            (PFN)DrvStartDoc           },
        {  INDEX_DrvEndDoc,              (PFN)DrvEndDoc             },
        {  INDEX_DrvEscape,              (PFN)DrvEscape             },
    };



#define TOTAL_DRVFUNC   (sizeof(DrvFuncTable)/sizeof(DrvFuncTable[0]))

extern "C" BOOL CALLBACK
DrvEnableDriver(ULONG          iEngineVersion,
        ULONG          cj,
        DRVENABLEDATA *pded)
{

     DUMPMSG("DrvEnableDriver");
     if (iEngineVersion < DDI_DRIVER_VERSION_NT4) {

          return(FALSE);
     }

     if (cj < sizeof(DRVENABLEDATA)) {

          return(FALSE);
     }
     pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
     pded->c = TOTAL_DRVFUNC;
     pded->pdrvfn = (DRVFN *)DrvFuncTable;

     return TRUE;

}
