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

public:
    static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);
    static LONG WINAPI CursorUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    static bool RestoreCursor();
    static void Initialize();
    bool ChangeCursor(const std::string& cursorFilePath);
    
private:
    static HCURSOR defaultCursor;
    static HCURSOR changedCursor;
};