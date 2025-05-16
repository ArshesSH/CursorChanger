#include "ProcessManager.h"

#include <tlhelp32.h>
#include <psapi.h>       // GetModuleFileNameEx, GetProcessMemoryInfo
#include <winternl.h>    // NT API 함수
#pragma comment(lib, "psapi.lib")

ProcessManager::ProcessManager()
{
}

ProcessManager::~ProcessManager()
{
}

DWORD ProcessManager::GetForegroundProcessIdOrZero()
{
    HWND hwnd = GetForegroundWindow();
    if (hwnd == nullptr)
    {
        return 0;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    return processId;
}

std::string ProcessManager::GetForegroundProcessNameOrEmpty()
{
    DWORD processId = GetForegroundProcessIdOrZero();
    if (processId == 0)
    {
        return "";
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess == nullptr)
    {
        return "";
    }

    wchar_t processPath[MAX_PATH];
    DWORD bufferSize = MAX_PATH;

    if (QueryFullProcessImageNameW(hProcess, 0, processPath, &bufferSize) == false)
    {
        CloseHandle(hProcess);
        return "";
    }

    CloseHandle(hProcess);

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
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
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

    CloseHandle(hSnapshot);
}

bool ProcessManager::IsProcessRunning(const std::wstring& processName) const
{
    for (const auto& process : processMap)
    {
        // compare process name
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
