
#include "precomp.h"

extern size_t maxsize;
#include <stdlib.h> 
#include <malloc.h>

#include "vphelper.h"
void GetMetaFileFromSpoolFile(TCHAR *SpoolFileName , LONG_PTR PageNbr , TCHAR *MetaFileName, PPDEV pPDev,LPBYTE *pDevmode)
{
     HANDLE   hFile = CreateFile( SpoolFileName,
             GENERIC_READ,              // open for reading 
             FILE_SHARE_READ,           // share for reading 
             NULL,                      // no security 
             OPEN_EXISTING,             // existing file only 
             FILE_ATTRIBUTE_NORMAL,     // normal file 
             NULL);                     // no attr. template 
     HANDLE hMapFile = CreateFileMapping(
             hFile,                       // handle to file
             NULL, // security
             PAGE_READONLY,                    // protection
             0,            // high-order DWORD of size
             0,             // low-order DWORD of size
             NULL                      // object name
             );
     LPBYTE pMapFile = (LPBYTE)MapViewOfFileEx(
             hMapFile,   // handle to file-mapping object
             FILE_MAP_READ,       // access mode
             0,      // high-order DWORD of offset
             0,       // low-order DWORD of offset
             0, // number of bytes to map
             NULL         // starting address
             );
     DWORD granularity = *((DWORD *)pMapFile);
     pMapFile +=  sizeof(DWORD);
     DWORD splheader = *((DWORD *)pMapFile);
     pMapFile +=  splheader;
     DWORD metafilelen = *((DWORD *)pMapFile);
     pMapFile +=  sizeof(DWORD); // This is hack after comparison with win9x.


     for(int Nbr = 1 ; Nbr < PageNbr ; Nbr++)
     {
          pMapFile += metafilelen;
          if(pPDev->pResetDC[Nbr - 1] == FALSE)
          {
               pMapFile += 20; 
          }
          else
          {
               /* skip the reset devmode here */
               pMapFile += 4; // This marker is the same as the one after Devmode
               DWORD offset = *((DWORD *)pMapFile); //this is multiple of 4bytes.
               pMapFile += offset + 4; // devmode + devmode-length
               pMapFile += 16 + 4; //Regular 20 bytes seperator(marker,...,metalen-tillhere,0000
               //,startpagemarker)
          }
          metafilelen = *((DWORD *)pMapFile);
          pMapFile += 4 ; //This has the metafile length!!!
          //keep incrementing till we are on the last page.
     }

     HANDLE   hMetaFile = CreateFile( MetaFileName,
             GENERIC_READ | GENERIC_WRITE,              // open for reading 
             FILE_SHARE_READ,           // share for reading 
             NULL,                      // no security 
             CREATE_ALWAYS,             // existing file only 
             FILE_ATTRIBUTE_NORMAL,     // normal file 
             NULL);                     // no attr. template 

     DWORD numWritten;
     WriteFile(
             hMetaFile,                                       // handle to output file
             pMapFile,                                   // data buffer
             metafilelen,                        // number of bytes to write
             &numWritten,
             NULL                          // overlapped buffer
             );

     if(pDevmode)
     {
          LPBYTE ptr = pMapFile + metafilelen;
          if(pPDev->pResetDC[PageNbr - 1] == TRUE)
          {
               ptr += 4; // This marker is the same as the one after Devmode
               DWORD offset = *((DWORD *)ptr); //this is multiple of 4bytes.
               *pDevmode = (LPBYTE)MALLOC(offset); 
               memcpy(*pDevmode , ptr + 4 , offset);
          }
     }
     CloseHandle(hMetaFile);

     UnmapViewOfFile( pMapFile   // starting address
             );
     CloseHandle(hMapFile);
     CloseHandle(hFile);
}

/*
 *   retrieve current job's spool file
 */

void GetSpoolFileName(DWORD JobId, TCHAR SpoolFileName[],HANDLE hDriver)
{

     DWORD cbNeeded;   
     DWORD dwType = REG_SZ;      // data type
     GetPrinterData(
             hDriver,    // handle to printer or print server
             SPLREG_DEFAULT_SPOOL_DIRECTORY,
             &dwType,      // data type
             NULL,       // configuration data buffer
             0,        // size of configuration data buffer
             &cbNeeded   // bytes received or required 
             );
     LPBYTE  pSpoolDirectory = (LPBYTE)MALLOC(cbNeeded); 
     GetPrinterData(
             hDriver,    // handle to printer or print server
             SPLREG_DEFAULT_SPOOL_DIRECTORY,
             &dwType,      // data type
             pSpoolDirectory,       // configuration data buffer
             cbNeeded,        // size of configuration data buffer
             &cbNeeded   // bytes received or required 
             );
     TCHAR TempSpoolFileName[MAX_PATH];
     {
          swprintf_s(TempSpoolFileName ,MAX_PATH,  L"%s\\",pSpoolDirectory); 
          TCHAR JobIdName[256];
          swprintf_s(JobIdName ,256,  L"%d",JobId);
          size_t zeros = 5 - wcslen(JobIdName);
          for(; zeros > 0; zeros--)
          {
               swprintf_s(SpoolFileName ,MAX_PATH,  L"%s0",TempSpoolFileName); 
               wcscpy_s(TempSpoolFileName, wcsnlen(SpoolFileName, maxsize)+1, SpoolFileName);
          }

          swprintf_s(SpoolFileName ,MAX_PATH,  L"%s%d.spl",TempSpoolFileName , JobId); 
         /* WIN32_FIND_DATA FindFileData;
          FindFirstFile(
                  SpoolFileName,               // file name
                  &FindFileData  // data buffer
                  );
          wcscpy_s(SpoolFileName , wcsnlen((TCHAR *)pSpoolDirectory, maxsize)+1, (TCHAR *)pSpoolDirectory);
          wcscat_s(SpoolFileName ,wcsnlen( L"\\", maxsize)+1,  L"\\");
		  wcscat_s(SpoolFileName ,wcsnlen( FindFileData.cFileName, maxsize)+1,  FindFileData.cFileName);
          
     */
	 }
     //RegCloseKey(hSpoolDirectory);
     free(pSpoolDirectory);
}

