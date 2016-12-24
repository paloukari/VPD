#define _DEBUG
#define DBG 1

#include <stddef.h>
#include <stdlib.h>
#include <windows.h>
#include <winddi.h>
#include <WINDOWSX.H>

#define _WINDEFP_NO_PDEVBRUSH
#include <winspool.h>

#include <string.h>
#include <intsafe.h>
#include <strsafe.h>
#include <COMMDLG.H>


#ifndef STRICT
#define STRICT
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef USERMODE_DRIVER
#define USERMODE_DRIVER
#endif

#ifdef _DEBUG
#ifdef USERMODE_DRIVER
#define DUMPMSG(msg) { \
	if(msg) \
	{ \
		OutputDebugStringA(msg); \
		OutputDebugStringA("\r\n");\
	}\
 }  
#else
#define DUMPMSG(msg)   
#endif
#else
#define DUMPMSG(msg)   
#endif