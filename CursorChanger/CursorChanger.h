#pragma once
#include <string>
#include <Windows.h>
#include <functional>

#include "Debugger.h"

class CursorChanger
{
public:
    static constexpr DWORD OCR_NORMAL =        32512;
    static constexpr DWORD OCR_IBEAM  =         32513;
    static constexpr DWORD OCR_WAIT   =         32514;
    static constexpr DWORD OCR_CROSS =          32515;
    static constexpr DWORD OCR_UP     =         32516;
    static constexpr DWORD OCR_HAND  =          32649;

public:
    CursorChanger();
    ~CursorChanger();

    static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);
    static LONG WINAPI CursorUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    static bool RestoreCursor();
    static bool IsCursorChanged();
    bool ChangeCursor(const std::string& cursorFilePath);
    
private:
    using SetSystemCursorFunc = BOOL(WINAPI*)(HCURSOR hCursor, DWORD id);
    using DestroyCursorFunc = BOOL(WINAPI*)(HCURSOR hCursor);
    using SystemParametersInfoWFunc = BOOL(WINAPI*)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
    using LoadCursorFromFileWFunc = HCURSOR(WINAPI*)(LPCWSTR lpFileName);
    
    static HCURSOR defaultCursor;
    static HCURSOR changedCursor;
    static SetSystemCursorFunc setSystemCursorFunc;
    static DestroyCursorFunc destroyCursorFunc;
    static SystemParametersInfoWFunc systemParametersInfoWFunc;
    static LoadCursorFromFileWFunc loadCursorFromFileWFunc;
};