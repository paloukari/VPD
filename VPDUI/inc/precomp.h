#define _DEBUG
#define DBG 1

#define _WINDEFP_NO_PDEVBRUSH

#include <stddef.h>
#include <stdarg.h>
#include <windows.h>
#include <winddi.h>
#include <winspool.h>
#include <winddiui.h>
#include <intsafe.h>
#include <strsafe.h>


#include <WINDOWSX.H>
#include <COMMDLG.H>


#ifndef NOGetHMODULE
extern HMODULE hPlotUIModule;
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