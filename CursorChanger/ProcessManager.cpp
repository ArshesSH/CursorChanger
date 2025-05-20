#include "ProcessManager.h"

#include <tlhelp32.h>
#include <winternl.h>
#include <cassert>

#include "DynamicLibraryLoader.h"
#pragma comment(lib, "psapi.lib")

ProcessManager::GetForegroundWindowFunc ProcessManager::getForegroundWindowFunc = nullptr;
ProcessManager::GetWindowThreadProcessIdFunc ProcessManager::getWindowThreadProcessIdFunc = nullptr;
ProcessManager::OpenProcessFunc ProcessManager::openProcessFunc = nullptr;
ProcessManager::QueryFullProcessImageNameWFunc ProcessManager::queryFullProcessImageNameWFunc = nullptr;
ProcessManager::CloseHandleFunc ProcessManager::closeHandleFunc = nullptr;
ProcessManager::CreateToolhelp32SnapshotFunc ProcessManager::createToolhelp32SnapshotFunc = nullptr;

ProcessManager::ProcessManager()
{
    getForegroundWindowFunc = DynamicLibraryLoader::GetFunctionOrNull<GetForegroundWindowFunc>(L"user32.dll", "GetForegroundWindow");
    assert(getForegroundWindowFunc != nullptr);
    getWindowThreadProcessIdFunc = DynamicLibraryLoader::GetFunctionOrNull<GetWindowThreadProcessIdFunc>(L"user32.dll", "GetWindowThreadProcessId");
    assert(getWindowThreadProcessIdFunc != nullptr);
    openProcessFunc = DynamicLibraryLoader::GetFunctionOrNull<OpenProcessFunc>(L"kernel32.dll", "OpenProcess");
    assert(openProcessFunc != nullptr);
    queryFullProcessImageNameWFunc = DynamicLibraryLoader::GetFunctionOrNull<QueryFullProcessImageNameWFunc>(L"kernel32.dll", "QueryFullProcessImageNameW");
    assert(queryFullProcessImageNameWFunc != nullptr);
    closeHandleFunc = DynamicLibraryLoader::GetFunctionOrNull<CloseHandleFunc>(L"kernel32.dll", "CloseHandle");
    assert(closeHandleFunc != nullptr);
    createToolhelp32SnapshotFunc = DynamicLibraryLoader::GetFunctionOrNull<CreateToolhelp32SnapshotFunc>(L"kernel32.dll", "CreateToolhelp32Snapshot");
    assert(createToolhelp32SnapshotFunc != nullptr);
}

ProcessManager::~ProcessManager()
{
    processMap.clear();
    getForegroundWindowFunc = nullptr;
    getWindowThreadProcessIdFunc = nullptr;
    openProcessFunc = nullptr;
}

DWORD ProcessManager::GetForegroundProcessIdOrZero()
{
    HWND hwnd = getForegroundWindowFunc();
    if (hwnd == nullptr)
    {
        return 0;
    }

    DWORD processId = 0;
    getWindowThreadProcessIdFunc(hwnd, &processId);
    return processId;
}

std::string ProcessManager::GetForegroundProcessNameOrEmpty()
{
    DWORD processId = GetForegroundProcessIdOrZero();
    if (processId == 0)
    {
        return "";
    }

    HANDLE hProcess = openProcessFunc(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess == nullptr)
    {
        return "";
    }

    wchar_t processPath[MAX_PATH];
    DWORD bufferSize = MAX_PATH;

    if (queryFullProcessImageNameWFunc(hProcess, 0, processPath, &bufferSize) == false)
    {
        closeHandleFunc(hProcess);
        return "";
    }

    closeHandleFunc(hProcess);

    std::wstring wProcessName(processPath);
    size_t lastSlash = wProcessName.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos)
    {
        wProcessName = wProcessName.substr(lastSlash + 1);
    }
    std::string processName(wProcessName.begin(), wProcessName.end());
    return processName;
}

void ProcessManager::UpdateProcesses()
{
    processMap.clear();
    
    HANDLE hSnapshot = createToolhelp32SnapshotFunc(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            processMap.insert_or_assign(pe32.th32ProcessID, pe32.szExeFile);
        } while (Process32Next(hSnapshot, &pe32));
    }

    closeHandleFunc(hSnapshot);
}

bool ProcessManager::IsProcessRunning(const std::wstring& processName) const
{
    for (const auto& process : processMap)
    {
        if (process.second == processName)
        {
            return true;
        }
    }
    
    return false;
}

bool ProcessManager::IsProcessRunning(const std::string& processName) const
{
    return IsProcessRunning(std::wstring(processName.begin(), processName.end()));
}

bool ProcessManager::IsProcessRunning(DWORD processId) const
{
    auto it = processMap.find(processId);
    if (it != processMap.end())
    {
        return true;
    }
    
    return false;
}
