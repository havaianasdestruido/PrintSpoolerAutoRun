// PrintSpoolerAutoRun.cpp
//
// Silent, windowless Win32 utility that checks the "Print Spooler" (spooler)
// service and starts it if it isn't running. Equivalent to running
// `net start spooler`, but as a double-clickable .exe with no console/window.
//
// Build (MSVC, x64 Native Tools Command Prompt):
//   cl /O2 /EHsc /DUNICODE /D_UNICODE PrintSpoolerAutoRun.cpp /link /SUBSYSTEM:WINDOWS Advapi32.lib
//
// Build (MinGW-w64):
//   g++ -O2 -municode -mwindows PrintSpoolerAutoRun.cpp -o PrintSpoolerAutoRun.exe -ladvapi32
//
// Notes:
//   - /SUBSYSTEM:WINDOWS (or -mwindows) is what suppresses the console window.
//   - Starting/querying a service requires the process to have sufficient
//     rights on the service. Standard users can usually query/start the
//     Print Spooler, but if it's locked down in your environment, run this
//     as admin or set it to auto-run at logon via Task Scheduler with
//     "Run with highest privileges".
//   - No UI is shown at all, success or failure, matching the "no-window
//     automatic replacement" request. Use the exit code if you need to know
//     the result (see ExitProcess calls below).

#include <windows.h>

// Exit codes (purely informational, since there's no console/UI):
//   0 = spooler already running, or started successfully
//   1 = could not open Service Control Manager
//   2 = could not open the spooler service
//   3 = could not query service status
//   4 = start request failed
//   5 = started but did not reach RUNNING state within the timeout

static void ExitWith(DWORD code)
{
    ExitProcess(code);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM)
        ExitWith(1);

    SC_HANDLE hService = OpenServiceW(
        hSCM,
        L"Spooler", // service name (not the display name "Print Spooler")
        SERVICE_QUERY_STATUS | SERVICE_START);

    if (!hService)
    {
        CloseServiceHandle(hSCM);
        ExitWith(2);
    }

    SERVICE_STATUS_PROCESS status{};
    DWORD bytesNeeded = 0;

    if (!QueryServiceStatusEx(
            hService,
            SC_STATUS_PROCESS_INFO,
            reinterpret_cast<LPBYTE>(&status),
            sizeof(status),
            &bytesNeeded))
    {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        ExitWith(3);
    }

    // Already running (or a start is already pending) -> nothing to do.
    if (status.dwCurrentState == SERVICE_RUNNING ||
        status.dwCurrentState == SERVICE_START_PENDING)
    {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        ExitWith(0);
    }

    if (!StartServiceW(hService, 0, nullptr))
    {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        ExitWith(4);
    }

    // Poll briefly until it's actually running (StartService returns
    // immediately once the start request is accepted).
    DWORD waited = 0;
    const DWORD timeoutMs = 10000;
    const DWORD pollMs = 250;

    while (waited < timeoutMs)
    {
        if (!QueryServiceStatusEx(
                hService,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&status),
                sizeof(status),
                &bytesNeeded))
        {
            break;
        }

        if (status.dwCurrentState == SERVICE_RUNNING)
        {
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCM);
            ExitWith(0);
        }

        Sleep(pollMs);
        waited += pollMs;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    ExitWith(5);
}