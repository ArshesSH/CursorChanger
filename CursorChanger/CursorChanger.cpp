#include "CursorChanger.h"

HCURSOR CursorChanger::defaultCursor = nullptr;
HCURSOR CursorChanger::changedCursor = nullptr;

CursorChanger::CursorChanger()
{
    if (defaultCursor == nullptr)
    {
        const auto curCursor = LoadCursor(nullptr, IDC_ARROW);
        defaultCursor = CopyCursor(curCursor);
        changedCursor = nullptr;
    }
}

CursorChanger::~CursorChanger()
{
    RestoreCursor();
    if (defaultCursor != nullptr)
    {
        DestroyCursor(defaultCursor);
        defaultCursor = nullptr;
    }
}

BOOL CursorChanger::ConsoleCtrlHandler(DWORD ctrlType)
{
    RestoreCursor();
    return FALSE;
}

LONG CursorChanger::CursorUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    RestoreCursor();
    return EXCEPTION_CONTINUE_SEARCH;
}

bool CursorChanger::RestoreCursor()
{
    if (defaultCursor == nullptr)
    {
        return false;
    }

    SetSystemCursor(defaultCursor, OCR_NORMAL);
    SystemParametersInfoW(SPI_SETCURSORS, 0, 0, 0);

    if (changedCursor != nullptr)
    {
        DestroyCursor(changedCursor);
        changedCursor = nullptr;
    }

    return true;
}

bool CursorChanger::ChangeCursor(const std::string& cursorFilePath)
{
    HCURSOR hCursor = LoadCursorFromFileW(std::wstring(cursorFilePath.begin(), cursorFilePath.end()).c_str());
    if (hCursor)
    {
        if (!SetSystemCursor(hCursor, OCR_NORMAL))
        {
            DestroyCursor(hCursor);
            Debugger::Log("Failed to set system cursor", Debugger::Type::Error);
            return false;
        }
        
        if (changedCursor != nullptr)
        {
            DestroyCursor(changedCursor);
        }

        changedCursor = hCursor;
        Debugger::Log("Cursor changed successfully");
        return true;
    }

    Debugger::Log("Failed to load cursor file", Debugger::Type::Error);
    return false;
}
