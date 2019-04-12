#if _WIN32
#include <windows.h>
#include <process.h>
#include <crtdbg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32
#if K4A_ENABLE_LEAK_DETECTION

// Enable this block for memory leak messaging to be send to STDOUT and debugger. This is useful for running a
// script to run all tests and quickly review all output. Also enable EXE's for verification with application
// verifier to to get call stacks of memory allocation. With memory leak addresses in hand use WinDbg command '!heap
// -p -a <Address>' to get the stack.
//
// NOTES:
//   Compile in debug mode.
//   Compile with K4A_ENABLE_LEAK_DETECTION set

BOOL WINAPI DllMain(_In_ HINSTANCE inst_dll, _In_ DWORD reason, _In_ LPVOID reserved)
{
    (void)inst_dll;
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
        /* Send memory leak detection error to stdout and debugger*/
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

        /* Do memory check at binary unload after statics are freed */
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        // Due to the flag _CRTDBG_LEAK_CHECK_DF, _CrtDumpMemoryLeaks() is called on unload
    }
    return TRUE;
}

#endif // K4A_ENABLE_LEAK_DETECTION
#else  //!_WIN32

/* This is needed so that this source file is not empty. */
typedef int make_c_compiler_happy_t;

#endif //_WIN32

#ifdef __cplusplus
}
#endif
