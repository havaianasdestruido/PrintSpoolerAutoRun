 --- PrintSpoolerAutoRun.cpp ---

Simple WIN32 tool for "fixing" (manually starting) Windows printer service (net start spooler).

It uses wWinMain with /SUBSYSTEM:WINDOWS, so it runs with no console/window at all; it just checks the Spooler service via the SCM API and calls StartService if it's stopped, then exits.

Build with MSVC:

cl /O2 /EHsc /DUNICODE /D_UNICODE PrintSpoolerAutoRun.cpp /link /SUBSYSTEM:WINDOWS Advapi32.lib


Or MinGW-w64:

g++ -O2 -municode -mwindows PrintSpoolerAutoRun.cpp -o PrintSpoolerAutoRun.exe -ladvapi32


Exit codes
(0=already running/started,
1-2=couldn't open SCM/service,
3=query failed,
4=start failed,
5=start pending timeout)