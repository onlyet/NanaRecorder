#include "dump.h"

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <DbgHelp.h>
#endif // WIN32

#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>

namespace  {
QString dirpath;
Dump::Callback_Dump after = nullptr;
}

#ifdef WIN32
static int generateMiniDump(PEXCEPTION_POINTERS pExceptionPointers)
{
#ifdef DyLoad_
    // Define function pointer
    typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
        );
    // Retrieve the "MiniDumpWriteDump" function from the "DbgHelp.dll" library
    MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
    HMODULE hDbgHelp = LoadLibrary(_T("DbgHelp.dll"));
    if (NULL == hDbgHelp)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

    if (NULL == pfnMiniDumpWriteDump)
    {
        FreeLibrary(hDbgHelp);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
#endif

    QDir::current().mkpath(dirpath);

    // Create a dmp file
    TCHAR szFileName[MAX_PATH] = { 0 };
    const QString path = QString("%1/%2-%3.dmp").arg(dirpath).arg(qApp->applicationName())
                             .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    path.toWCharArray(szFileName);

    HANDLE hDumpFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    if (INVALID_HANDLE_VALUE == hDumpFile)
    {
#ifdef DyLoad_
        FreeLibrary(hDbgHelp);
#endif
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    // Write to dump file
    MINIDUMP_EXCEPTION_INFORMATION expParam;
    expParam.ThreadId = GetCurrentThreadId();
    expParam.ExceptionPointers = pExceptionPointers;
    expParam.ClientPointers = FALSE;
#ifdef DyLoad_
    pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                         hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &expParam : NULL), NULL, NULL);
#else
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                      hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &expParam : NULL), NULL, NULL);
#endif
    // Release files
    CloseHandle(hDumpFile);
#ifdef DyLoad_
    FreeLibrary(hDbgHelp);
#endif

#if  0
    MessageBox(NULL, TEXT("The client has crashed; please contact technical support."), TEXT("Crash Notification"), 0);
#endif // _WIN32

    qCritical() << "Crash!!! file:" << QString::fromWCharArray(szFileName);


    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI exceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
    // Perform exception filtering or provide notifications here.
    if (IsDebuggerPresent())
    {
		// Hand over to the debugger; the debugger displays the exception window.
        return EXCEPTION_CONTINUE_SEARCH;
    }
    auto ret = generateMiniDump(lpExceptionInfo);
    if (after) after();
    return ret;
}

// Once this function is successfully called, subsequent calls to SetUnhandledExceptionFilter will have no effect.
static void disableSetUnhandledExceptionFilter()
{
    void* addr = (void*)GetProcAddress(LoadLibrary(L"kernel32.dll"),
                                         "SetUnhandledExceptionFilter");

    if (addr)
    {
        unsigned char code[16];
        int size = 0;

        code[size++] = 0x33;
        code[size++] = 0xC0;
        code[size++] = 0xC2;
        code[size++] = 0x04;
        code[size++] = 0x00;

        DWORD dwOldFlag, dwTempFlag;
        VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
        WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
        VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
    }
}
#endif

void Dump::Init(const QString &dirpath_, Dump::Callback_Dump after_)
{
#ifdef WIN32
	dirpath = dirpath_;
	after = after_;

	//bool useVectoredExceptionHandler = mSetting.value(UseVectoredExceptionHandler, false).toBool();
	//if (useVectoredExceptionHandler)
	//{
		//AddVectoredExceptionHandler(1, exceptionFilter);
	//}
	//else
	//{
	SetUnhandledExceptionFilter(exceptionFilter);
	//    //DisableSetUnhandledExceptionFilter();
	//}
#else

#endif // !WIN32

}
