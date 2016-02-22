// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2009 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   Exception handling for Windows, because the SDL parachute sucks.
//
//   Based loosely on ExceptionHandler.cpp
//   Author:  Hans Dietrich
//            hdietrich2@hotmail.com
//   Original license: public domain
//
//-----------------------------------------------------------------------------

#ifndef _WIN32
#error i_exception.c is for Win32 only
#endif

#include <windows.h>
#include <tchar.h>

//=============================================================================
//
// Macros
//

#define charcount(arr) (sizeof(arr) / sizeof(arr[0]))
#define LOG_BUFFER_SIZE 8192
#define MEGABYTE (1024*1024)
#define RoundMem(amt) (((amt) + MEGABYTE - 1) / MEGABYTE)
#define CODETOPRINT 16
#define STACKTOPRINT 3072

//=============================================================================
//
// Data
//

static int    exceptionCaught = 0;
static TCHAR  moduleFileName[MAX_PATH*2];
static TCHAR  moduleName[MAX_PATH*2];
static TCHAR  crashModulePath[MAX_PATH*2];
static HANDLE logFile;
static TCHAR  logbuffer[LOG_BUFFER_SIZE];
static int    logidx;
static TCHAR *fileName;

static PEXCEPTION_RECORD exceptionRecord;
static PCONTEXT          contextRecord;

//=============================================================================
//
// Static Routines
//

//
// lstrrchr
//
static TCHAR *lstrrchr(LPCTSTR str, int ch) {
    TCHAR *start = (TCHAR *)str;

    while(*str++); // find end

    // search backward
    while(--str != start && *str != (TCHAR)ch);

    if(*str == (TCHAR)ch) {
        return (TCHAR *)str;    // found it
    }

    return NULL;
}

//
// ExtractFileName
//
// Gets the filename from a path string
//
static TCHAR *ExtractFileName(LPCTSTR path) {
    TCHAR *ret;

    // look for last instance of a backslash
    if((ret = lstrrchr(path, _T('\\')))) {
        ++ret;
    }
    else {
        ret = (TCHAR *)path;
    }

    return ret;
}

//
// GetModuleName
//
// Gets the module file name as a full path, and extracts the file portion.
//
static void GetModuleName(void) {
    TCHAR *dot;

    ZeroMemory(moduleFileName, sizeof(moduleFileName));

    if(GetModuleFileName(NULL, moduleFileName, charcount(moduleFileName) - 2) <= 0) {
        lstrcpy(moduleFileName, _T("Unknown"));
    }

    fileName = ExtractFileName(moduleFileName);
    lstrcpy(moduleName, fileName);

    // find extension and remove it
    if((dot = lstrrchr(moduleName, _T('.')))) {
        dot[0] = 0;
    }

    // put filename onto module path
    lstrcpy(fileName, _T("CRASHLOG.TXT"));
}

//=============================================================================
//
// Log File Stuff
//

//
// OpenLogFile
//
// Opens the exception log file.
//
static int OpenLogFile(void) {
    logFile = CreateFile(moduleFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);

    if(logFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    else {
        // set to append
        SetFilePointer(logFile, 0, 0, FILE_END);
        return 1;
    }
}

//
// LogFlush
//
// To avoid too many WriteFile calls, we'll buffer the output.
//
static void LogFlush(HANDLE file) {
    DWORD bytecount;

    if(logidx > 0) {
        WriteFile(file, logbuffer, lstrlen(logbuffer), &bytecount, 0);
        logidx = 0;
    }
}

//
// LogPrintf
//
// This is supposed to be safe to call during an exception, unlike fprintf.
// We'll see.
//
static void LogPrintf(LPCTSTR fmt, ...) {
    DWORD bytecount;
    va_list args;

    va_start(args, fmt);

    if(logidx > LOG_BUFFER_SIZE - 1024) {
        WriteFile(logFile, logbuffer, lstrlen(logbuffer), &bytecount, 0);
        logidx = 0;
    }

    logidx += wvsprintf(&logbuffer[logidx], fmt, args);

    va_end(args);
}

//=============================================================================
//
// Information Output Routines
//

typedef struct exceptiondata_s {
    DWORD  code;
    TCHAR *name;
} exceptiondata_t;

static exceptiondata_t ExceptionData[] = {
    { 0x40010005, _T("a Ctrl+C")                     },
    { 0x40010008, _T("a Ctrl+Brk")                   },
    { 0x80000002, _T("a Data Type Misalignment")     },
    { 0x80000003, _T("a Breakpoint")                 },
    { 0xC0000005, _T("an Access Violation")          },
    { 0xC0000006, _T("an In-Page Error")             },
    { 0xC0000017, _T("a No Memory")                  },
    { 0xC000001D, _T("an Illegal Instruction")       },
    { 0xC0000025, _T("a Non-continuable Exception")  },
    { 0xC0000026, _T("an Invalid Disposition")       },
    { 0xC000008C, _T("an Array Bounds Exceeded")     },
    { 0xC000008D, _T("a Float Denormal Operand")     },
    { 0xC000008E, _T("a Float Divide by Zero")       },
    { 0xC000008F, _T("a Float Inexact Result")       },
    { 0xC0000090, _T("a Float Invalid Operation")    },
    { 0xC0000091, _T("a Float Overflow")             },
    { 0xC0000092, _T("a Float Stack Check")          },
    { 0xC0000093, _T("a Float Underflow")            },
    { 0xC0000094, _T("an Integer Divide by Zero")    },
    { 0xC0000095, _T("an Integer Overflow")          },
    { 0xC0000096, _T("a Privileged Instruction")     },
    { 0xC00000FD, _T("a Stack Overflow")             },
    { 0xC0000142, _T("a DLL Initialization Failure") },
    { 0xE06D7363, _T("a Microsoft C++")              },

    // must be last
    { 0, NULL }
};

//
// PhraseForException
//
// Gets a text string describing the type of exception that occurred.
//
static const TCHAR *PhraseForException(DWORD code) {
    const TCHAR *ret    = _T("an Unknown");
    exceptiondata_t *ed = ExceptionData;

    while(ed->name) {
        if(code == ed->code) {
            ret = ed->name;
            break;
        }
        ++ed;
    }

    return ret;
}

//
// PrintHeader
//
// Prints a header for the exception.
//
static void PrintHeader(void) {
    TCHAR *crashModuleFn = _T("Unknown");
    MEMORY_BASIC_INFORMATION memoryInfo;

    ZeroMemory(crashModulePath, sizeof(crashModulePath));

#ifdef _M_IX86
    // Use VirtualQuery to retrieve the allocation base associated with the
    // process's code address.
    if(VirtualQuery((void *)contextRecord->Eip, &memoryInfo, sizeof(memoryInfo))) {
        if(GetModuleFileName((HINSTANCE)memoryInfo.AllocationBase,
                             crashModulePath, charcount(crashModulePath)-2) > 0) {
            crashModuleFn = ExtractFileName(crashModulePath);
        }
    }

    LogPrintf(
        _T("%s caused %s Exception (0x%08x)\r\nin module %s at %04x:%08x.\r\n\r\n"),
        moduleName,
        PhraseForException(exceptionRecord->ExceptionCode),
        exceptionRecord->ExceptionCode,
        crashModuleFn,
        contextRecord->SegCs,
        contextRecord->Eip);
#else
    // FIXME: how to get crash module name and address on non-x86, x64?
    LogPrintf(
        _T("%s caused %s Exception (0x%08x)\r\n\r\n"),
        moduleName,
        PhraseForException(exceptionRecord->ExceptionCode),
        exceptionRecord->ExceptionCode);
#endif
}

//
// MakeTimeString
//
static void MakeTimeString(FILETIME time, LPTSTR str) {
    WORD d, t;

    str[0] = _T('\0');

    if(FileTimeToLocalFileTime(&time, &time) &&
            FileTimeToDosDateTime(&time, &d, &t)) {
        wsprintf(str, _T("%d/%d/%d %02d:%02d:%02d"),
                 (d / 32) & 15, d & 31, d / 512 + 1980,
                 t >> 11, (t >> 5) & 0x3F, (t & 0x1F) * 2);
    }
}

//
// PrintTime
//
// Prints the time at which the exception occurred.
//
static void PrintTime(void) {
    FILETIME crashtime;
    TCHAR    timestr[256];

    GetSystemTimeAsFileTime(&crashtime);
    MakeTimeString(crashtime, timestr);

    LogPrintf(_T("Error occurred at %s.\r\n"), timestr);
}

//
// PrintUserInfo
//
// Prints out info on the user running the process.
//
static void PrintUserInfo(void) {
    TCHAR moduleName[MAX_PATH * 2];
    TCHAR userName[256];
    DWORD userNameLen;

    ZeroMemory(moduleName, sizeof(moduleName));
    ZeroMemory(userName,   sizeof(userName));
    userNameLen = charcount(userName) - 2;

    if(GetModuleFileName(0, moduleName, charcount(moduleName) - 2) <= 0) {
        lstrcpy(moduleName, _T("Unknown"));
    }

    if(!GetUserName(userName, &userNameLen)) {
        lstrcpy(userName, _T("Unknown"));
    }

    LogPrintf(_T("%s, run by %s.\r\n"), moduleName, userName);
}

//
// PrintOSInfo
//
// Prints out version information for the operating system.
//
static void PrintOSInfo(void) {
    OSVERSIONINFO osinfo;
    TCHAR         mmb[64];

    ZeroMemory(mmb, sizeof(mmb));

    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(GetVersionEx(&osinfo)) {
        DWORD platformId   = osinfo.dwPlatformId;
        DWORD minorVersion = osinfo.dwMinorVersion;
        DWORD majorVersion = osinfo.dwMajorVersion;
        DWORD buildNumber  = osinfo.dwBuildNumber & 0xFFFF;

        wsprintf(mmb, _T("%u.%u.%u"), majorVersion, minorVersion, buildNumber);

        LogPrintf("Operating system: %s\r\n", mmb);
    }
    else {
        LogPrintf(_T("%s"), "Operating system unknown\r\n");
    }
}

//
// PrintCPUInfo
//
// Prints information on the user's CPU(s)
//
static void PrintCPUInfo(void) {
    SYSTEM_INFO   sysinfo;

    GetSystemInfo(&sysinfo);

    LogPrintf(_T("%d processor%s, type %d.\r\n"),
              sysinfo.dwNumberOfProcessors,
              sysinfo.dwNumberOfProcessors > 1 ? _T("s") : _T(""),
              sysinfo.dwProcessorType);
}

//
// PrintMemInfo
//
// Prints memory usage information.
//
static void PrintMemInfo(void) {
    MEMORYSTATUS meminfo;

    meminfo.dwLength = sizeof(meminfo);

    GlobalMemoryStatus(&meminfo);

    LogPrintf(_T("%d%% memory in use.\r\n"), meminfo.dwMemoryLoad);
    LogPrintf(_T("%d MB physical memory.\r\n"),
              RoundMem(meminfo.dwTotalPhys));
    LogPrintf(_T("%d MB physical memory free.\r\n"),
              RoundMem(meminfo.dwAvailPhys));
    LogPrintf(_T("%d MB page file.\r\n"),
              RoundMem(meminfo.dwTotalPageFile));
    LogPrintf(_T("%d MB paging file free.\r\n"),
              RoundMem(meminfo.dwAvailPageFile));
    LogPrintf(_T("%d MB user address space.\r\n"),
              RoundMem(meminfo.dwTotalVirtual));
    LogPrintf(_T("%d MB user address space free.\r\n"),
              RoundMem(meminfo.dwAvailVirtual));
}

//
// PrintSegVInfo
//
// For access violations, this prints additional information.
//
static PrintSegVInfo(void) {
    TCHAR msg[1024];
    const TCHAR* readOrWrite = _T("read");

    if(exceptionRecord->ExceptionInformation[0]) {
        readOrWrite = _T("written");
    }

    wsprintf(msg, _T("Access violation at %08x. The memory could not be %s.\r\n"),
             exceptionRecord->ExceptionInformation[1], readOrWrite);

    LogPrintf(_T("%s"), msg);
}

// Note: everything inside here is x86-specific, unfortunately.
#ifdef _M_IX86

//
// PrintRegInfo
//
// Prints registers.
//
static void PrintRegInfo(void) {
    LogPrintf(_T("\r\nContext:\r\n"));
    LogPrintf(_T("EDI:    0x%08x  ESI: 0x%08x  EAX:   0x%08x\r\n"),
              contextRecord->Edi, contextRecord->Esi, contextRecord->Eax);
    LogPrintf(_T("EBX:    0x%08x  ECX: 0x%08x  EDX:   0x%08x\r\n"),
              contextRecord->Ebx, contextRecord->Ecx, contextRecord->Edx);
    LogPrintf(_T("EIP:    0x%08x  EBP: 0x%08x  SegCs: 0x%08x\r\n"),
              contextRecord->Eip, contextRecord->Ebp, contextRecord->SegCs);
    LogPrintf(_T("EFlags: 0x%08x  ESP: 0x%08x  SegSs: 0x%08x\r\n"),
              contextRecord->EFlags, contextRecord->Esp, contextRecord->SegSs);
}

//
// PrintCS
//
// Prints out some of the memory at the instruction pointer.
//
static void PrintCS(void) {
    BYTE *ipaddr = (BYTE *)contextRecord->Eip;
    int i;

    LogPrintf(_T("\r\nBytes at CS:EIP:\r\n"));

    for(i = 0; i < CODETOPRINT; ++i) {
        // must check for exception, in case of invalid instruction pointer
        __try {
            LogPrintf(_T("%02x "), ipaddr[i]);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            LogPrintf(_T("?? "));
        }
    }
}

//
// PrintStack
//
// Prints out the contents of the stack.
//
static void PrintStack(void) {
    DWORD *stackptr = (DWORD *)contextRecord->Esp;
    DWORD *stacktop;
    DWORD *stackstart;
    int   cnt = 0;
    int   numPrinted = 0;

    LogPrintf(_T("\r\n\r\nStack:\r\n"));

    __try {
        __asm {
            // Load the top address of the stack from the thread information block.
            mov   eax, fs:[4]
            mov   stacktop, eax
        }

        if(stacktop > stackptr + STACKTOPRINT) {
            stacktop = stackptr + STACKTOPRINT;
        }

        stackstart = stackptr;

        while(stackptr + 1 <= stacktop) {
            if((cnt % 4) == 0) {
                stackstart = stackptr;
                numPrinted = 0;
                LogPrintf(_T("0x%08x: "), stackptr);
            }

            if((++cnt % 4) == 0 || stackptr + 2 > stacktop) {
                int i, n;

                LogPrintf(_T("%08x "), *stackptr);
                ++numPrinted;

                n = numPrinted;

                while(n < 4) {
                    LogPrintf(_T("         "));
                    ++n;
                }

                for(i = 0; i < numPrinted; ++i) {
                    int j;
                    DWORD stackint = *stackstart;

                    for(j = 0; j < 4; ++j) {
                        char c = (char)(stackint & 0xFF);
                        if(c < 0x20 || c > 0x7E) {
                            c = '.';
                        }
                        LogPrintf(_T("%c"), c);
                        stackint = stackint >> 8;
                    }
                    ++stackstart;
                }

                LogPrintf(_T("\r\n"));
            }
            else {
                LogPrintf(_T("%08x "), *stackptr);
                ++numPrinted;
            }
            ++stackptr;
        }

        LogPrintf(_T("\r\n"));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        LogPrintf(_T("Could not access stack.\r\n"));
    }
}

#endif // _M_IX86

//=============================================================================
//
// Main Routine
//

//
// I_W32ExceptionHandler
//
// Main routine for handling Win32 exceptions.
//
int __cdecl I_W32ExceptionHandler(PEXCEPTION_POINTERS ep) {
    // if this happens, it means the exception handler crashed. Uh-oh!
    if(exceptionCaught) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    exceptionCaught = 1;

    // set exception and context pointers
    exceptionRecord = ep->ExceptionRecord;
    contextRecord   = ep->ContextRecord;

    // get module path and name information
    GetModuleName();

    // open exception log
    if(!OpenLogFile()) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    PrintHeader();    // output header
    PrintTime();      // time of exception
    PrintUserInfo();  // print user information
    PrintOSInfo();    // print operating system information
    PrintCPUInfo();   // print processor information
    PrintMemInfo();   // print memory usage

    // print additional info for access violations
    if(exceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
            exceptionRecord->NumberParameters >= 2) {
        PrintSegVInfo();
    }

    // This won't be terribly useful on non-x86 as-is.
    // That is assuming it works at all, of course.
#ifdef _M_IX86
    PrintRegInfo();    // print CPU registers
    PrintCS();         // print code segment at EIP
    PrintStack();      // print stack dump
#endif

    LogPrintf(_T("\r\n===== [end of %s] =====\r\n"), _T("CRASHLOG.TXT"));
    LogFlush(logFile);
    CloseHandle(logFile);

    return EXCEPTION_EXECUTE_HANDLER;
}


