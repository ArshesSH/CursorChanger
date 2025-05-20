#include "CursorChanger.h"

#include <assert.h>

#include "DynamicLibraryLoader.h"
#include "StringUtils.h"


HCURSOR CursorChanger::defaultCursor = nullptr;
HCURSOR CursorChanger::changedCursor = nullptr;
CursorChanger::SetSystemCursorFunc CursorChanger::setSystemCursorFunc = nullptr;
CursorChanger::DestroyCursorFunc CursorChanger::destroyCursorFunc = nullptr;
CursorChanger::SystemParametersInfoWFunc CursorChanger::systemParametersInfoWFunc = nullptr;
CursorChanger::LoadCursorFromFileWFunc CursorChanger::loadCursorFromFileWFunc = nullptr;

CursorChanger::CursorChanger()
{
    if (defaultCursor == nullptr)
    {
        const auto curCursor = LoadCursor(nullptr, IDC_ARROW);
        defaultCursor = CopyCursor(curCursor);
        changedCursor = nullptr;
    }

    setSystemCursorFunc = DynamicLibraryLoader::GetFunctionOrNull<SetSystemCursorFunc>(L"user32.dll", "SetSystemCursor");
    assert(setSystemCursorFunc != nullptr);
    destroyCursorFunc = DynamicLibraryLoader::GetFunctionOrNull<DestroyCursorFunc>(L"user32.dll", "DestroyCursor");
    assert(destroyCursorFunc != nullptr);
    systemParametersInfoWFunc = DynamicLibraryLoader::GetFunctionOrNull<SystemParametersInfoWFunc>(L"user32.dll", "SystemParametersInfoW");
    assert(systemParametersInfoWFunc != nullptr);
    loadCursorFromFileWFunc = DynamicLibraryLoader::GetFunctionOrNull<LoadCursorFromFileWFunc>(L"user32.dll", "LoadCursorFromFileW");
    assert(loadCursorFromFileWFunc != nullptr);
}

CursorChanger::~CursorChanger()
{
    RestoreCursor();
    if (defaultCursor != nullptr)
    {
        destroyCursorFunc(defaultCursor);
        defaultCursor = nullptr;
    }

    setSystemCursorFunc = nullptr;
    destroyCursorFunc = nullptr;
    systemParametersInfoWFunc = nullptr;
    loadCursorFromFileWFunc = nullptr;
    defaultCursor = nullptr;
    changedCursor = nullptr;
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

    setSystemCursorFunc(defaultCursor, OCR_NORMAL);
    systemParametersInfoWFunc(SPI_SETCURSORS, 0, 0, 0);

    if (changedCursor != nullptr)
    {
        destroyCursorFunc(changedCursor);
        changedCursor = nullptr;
    }

    return true;
}

bool CursorChanger::IsCursorChanged()
{
    return changedCursor != nullptr;
}

bool CursorChanger::ChangeCursor(const std::string& cursorFilePath)
{
    std::wstring wCursorFilePath = StringUtils::Utf8ToWide(cursorFilePath);
    HCURSOR hCursor = loadCursorFromFileWFunc(wCursorFilePath.c_str());
    if (hCursor)
    {
        if (!setSystemCursorFunc(hCursor, OCR_NORMAL))
        {
            destroyCursorFunc(hCursor);
            Debugger::Log("Failed to set system cursor", Debugger::Type::Error);
            return false;
        }
        
        if (changedCursor != nullptr)
        {
            destroyCursorFunc(changedCursor);
        }

        changedCursor = hCursor;
        Debugger::Log("Cursor changed successfully");
        return true;
    }

    Debugger::Log("Failed to load cursor file", Debugger::Type::Error);
    return false;
}
