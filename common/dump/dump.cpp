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
    // 定义函数指针
    typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
        );
    // 从 "DbgHelp.dll" 库中获取 "MiniDumpWriteDump" 函数
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

    // 创建 dmp 文件
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
    // 写入 dmp 文件
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
    // 释放文件
    CloseHandle(hDumpFile);
#ifdef DyLoad_
    FreeLibrary(hDbgHelp);
#endif

#if  0
    MessageBox(NULL, TEXT("客户端崩溃了，请联系技术支持人员"), TEXT("崩溃提示"), 0);
#endif // _WIN32

    qCritical() << "Crash!!! file:" << QString::fromWCharArray(szFileName);


    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI exceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
    // 这里做一些异常的过滤或提示
    if (IsDebuggerPresent())
    {
		// 交给调试器处理，调试器显示异常窗口
        return EXCEPTION_CONTINUE_SEARCH;
    }
    auto ret = generateMiniDump(lpExceptionInfo);
    if (after) after();
    return ret;
}

// 此函数一旦成功调用，之后对 SetUnhandledExceptionFilter 的调用将无效
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
