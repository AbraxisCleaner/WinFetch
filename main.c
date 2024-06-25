#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <memory.h>
#include <intrin.h>
#include <string.h>
#include <dxgi1_3.h>

#include <VersionHelpers.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define ZERO_THAT(x) memset(x, 0, sizeof(*x))
#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) ((x * 1024) * 1024)
#define GIGABYTES(x) (((x * 1024) * 1024) * 1024)
#define BYTES_TO_KILOBYTES(x) (x / 1024)
#define BYTES_TO_MEGABYTES(x) ((x / 1024) / 1024)
#define BYTES_TO_GIGABYTES(x) (((x / 1024) / 1024) / 1024)

// FORWARD ---------------------------------------------------------
HANDLE _stdout;
HINSTANCE _hinstance;

void log(const char *fmt, ...);
#define ASSERT(x) if (!(x)) { log("------ ASSERTION FAILED ------\n%s\nFile: %s, line %d\n", #x, __FILE__, __LINE__); }

BOOL monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT lprect, LPARAM lparam);

// MAIN ------------------------------------------------------------
int main(int argc, char **argv) {
    _hinstance = GetModuleHandleA(NULL);
    
    _stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (_stdout == (HANDLE)-1)
        return -1;

    int b_all = FALSE;
    if (argc > 1 && !strcmp(argv[1], "-all")) {
        b_all = TRUE;
    }

    log("---------- System Information ----------\n");
    MEMORYSTATUSEX memstatus = { sizeof(memstatus) };
    SYSTEM_INFO sysinfo;
    GlobalMemoryStatusEx(&memstatus);
    GetSystemInfo(&sysinfo);

    // -- System Metrics
    if (IsWindows10OrGreater())
        log("                OS: Windows 10\n");
    else if (IsWindows8Point1OrGreater)
        log("                OS: Windows 8.1\n");
    else if (IsWindows8OrGreater)
        log("                OS: Windows 8\n");
    else if (IsWindows7SP1OrGreater)
        log("                OS: Windows 7 SP1\n");
    else if (IsWindows7OrGreater)
        log("                OS: Windows 7\n");
    else if (IsWindowsVistaSP2OrGreater)
        log("                OS: Windows Vista SP2\n");
    else if (IsWindowsVistaSP1OrGreater)
        log("                OS: Windows Vista SP1\n");
    else if (IsWindowsVistaOrGreater)
        log("                OS: Windows Vista\n");
    else if (IsWindowsXPSP3OrGreater)
        log("                OS: Windows XP SP3\n");
    else if (IsWindowsXPSP2OrGreater)
        log("                OS: Windows XP SP2\n");
    else if (IsWindowsXPSP1OrGreater)
        log("                OS: Windows XP SP1\n");
    else if (IsWindowsXPOrGreater)
        log("                OS: Windows XP\n");

    log("Logical processors: %d\n", sysinfo.dwNumberOfProcessors);

    switch (sysinfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_ARM64: log("      Architecture: ARM64\n"); break;
        case PROCESSOR_ARCHITECTURE_ARM: log("      Architecture: ARM\n"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: log("      Architecture: Intel\n"); break;
        case PROCESSOR_ARCHITECTURE_AMD64: log("      Architecture: AMD64 (x64)\n"); break;
        case PROCESSOR_ARCHITECTURE_MIPS: log("      Architecture: MIPS\n"); break;
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default: log("      Architecture: Unknown\n"); break;
    }
    
    log("   Physical memory: %lld/%lldgb\n", BYTES_TO_GIGABYTES(memstatus.ullAvailPhys), BYTES_TO_GIGABYTES(memstatus.ullTotalPhys));
    log("    Virtual memory: %lld/%lldgb\n", BYTES_TO_GIGABYTES(memstatus.ullAvailVirtual), BYTES_TO_GIGABYTES(memstatus.ullTotalVirtual));
    log("         Page Size: %d\n", sysinfo.dwPageSize);

    if (b_all) {
        log("    Display Devices [\n");
        DISPLAY_DEVICEA dda = { sizeof(dda) };
        for (int i = 0; EnumDisplayDevicesA(NULL, i, &dda, 0) != 0; ++i) {
            log("\t\t%s | %s\n", dda.DeviceName, dda.DeviceString);
        }
        log("\t]\n");

        log("           Monitors [\n"); 
        int monitor_index = 0;
        while (TRUE) {
            if (!EnumDisplayMonitors(NULL, NULL, monitor_enum_callback, (LONG_PTR)(&monitor_index)));
                break;
        }
        log("\t]\n");    
    } else {
        IDXGIFactory2 *dxgi_factory;
        CreateDXGIFactory1(&IID_IDXGIFactory2, &dxgi_factory);

        IDXGIAdapter1 *adapter;
        DXGI_ADAPTER_DESC1 ad1 = {0}, ad2 = {0};
        for (int i = 0; dxgi_factory->lpVtbl->EnumAdapters1(dxgi_factory, i, &adapter) == S_OK; ++i) {
            adapter->lpVtbl->GetDesc1(adapter, &ad2);
            if (ad2.DedicatedVideoMemory > ad1.DedicatedVideoMemory) {
                ad1 = ad2;
            }
            adapter->lpVtbl->Release(adapter);
        }

        char mbcs[128];
        size_t mbcs_len;
        wcstombs_s(&mbcs_len, mbcs, 128, ad1.Description, sizeof(ad1.Description));
        log("    Display Device: %s | %lldgb\n", mbcs, BYTES_TO_GIGABYTES(ad1.DedicatedVideoMemory));

        MONITORINFO monitor = { sizeof(monitor) };
        POINT pt = {0};
        GetMonitorInfoA(MonitorFromPoint(pt, 1), &monitor);
        log("           Monitor: %dx%d\n", (monitor.rcWork.right - monitor.rcWork.left), (monitor.rcWork.bottom - monitor.rcWork.top));

        dxgi_factory->lpVtbl->Release(dxgi_factory);
    }

    return NO_ERROR;
}

BOOL monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT lprect, LPARAM lparam)
{
    MONITORINFO monitor = { sizeof(monitor) };
    GetMonitorInfoA(handle, &monitor);
    
    log("\t\t%d: %dx%d\n", *(int *)(LONG_PTR)lparam, monitor.rcWork.right - monitor.rcWork.left, monitor.rcWork.bottom - monitor.rcWork.top);
    ++(*((int *)(LONG_PTR)lparam));

    return NO_ERROR;
}

void log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int bufsize = 256;
    int writesize = 0;
    char *buffer = NULL;

    while (TRUE) {
        buffer = (char *)malloc(bufsize);
        writesize = stbsp_vsprintf(buffer, fmt, args);
        if (writesize)
            break;
        free(buffer);
        bufsize += 256;
    }

    WriteFile(_stdout, buffer, writesize, NULL, NULL);
    free(buffer);
}
