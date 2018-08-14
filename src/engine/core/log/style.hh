#ifndef __STYLE__70700898
#define __STYLE__70700898

#define ANSI_BLACK   "\x1b[30m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_WHITE   "\x1b[37m"

#define ANSI_BLACK_BOLD   "\x1b[1;30m"
#define ANSI_RED_BOLD     "\x1b[1;31m"
#define ANSI_GREEN_BOLD   "\x1b[1;32m"
#define ANSI_YELLOW_BOLD  "\x1b[1;33m"
#define ANSI_BLUE_BOLD    "\x1b[1;34m"
#define ANSI_MAGENTA_BOLD "\x1b[1;35m"
#define ANSI_CYAN_BOLD    "\x1b[1;36m"
#define ANSI_WHITE_BOLD   "\x1b[1;37m"

#define ANSI_RESET "\x1b[0m"

/* Normal colors */
#define sBLACK(_Str)   ANSI_BLACK _Str ANSI_RESET
#define sRED(_Str)     ANSI_RED _Str ANSI_RESET
#define sGREEN(_Str)   ANSI_GREEN _Str ANSI_RESET
#define sYELLOW(_Str)  ANSI_YELLOW _Str ANSI_RESET
#define sBLUE(_Str)    ANSI_BLUE _Str ANSI_RESET
#define sMAGENTA(_Str) ANSI_MAGENTA _Str ANSI_RESET
#define sCYAN(_Str)    ANSI_CYAN _Str ANSI_RESET
#define sWHITE(_Str)   ANSI_WHITE _Str ANSI_RESET

/* Bold variants */
#define sBLACKB(_Str)   ANSI_BLACK_BOLD _Str ANSI_RESET
#define sREDB(_Str)     ANSI_RED_BOLD _Str ANSI_RESET
#define sGREENB(_Str)   ANSI_GREEN_BOLD _Str ANSI_RESET
#define sYELLOWB(_Str)  ANSI_YELLOW_BOLD _Str ANSI_RESET
#define sBLUEB(_Str)    ANSI_BLUE_BOLD _Str ANSI_RESET
#define sMAGENTAB(_Str) ANSI_MAGENTA_BOLD _Str ANSI_RESET
#define sCYANB(_Str)    ANSI_CYAN_BOLD _Str ANSI_RESET
#define sWHITEB(_Str)   ANSI_WHITE_BOLD _Str ANSI_RESET

#endif //__STYLE__70700898
