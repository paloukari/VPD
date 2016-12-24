
enum Layout { UP1 ,UP2,UP4,UP6,UP9,BOOKLET};
struct VPDEVMODE{
     WCHAR PrinterName[256];
     Layout lyt;
     WORD PaperSize;
     GDIINFO  gi;
     int numcolors;
     ULONG Palette[256];
     LOGFONT lf;
	 BOOL Preview;
	 BOOL PrintToPaper;
         BYTE Complevel;
};
struct VDEVMODE{
     DEVMODEW dm;
     VPDEVMODE pdm;
};
