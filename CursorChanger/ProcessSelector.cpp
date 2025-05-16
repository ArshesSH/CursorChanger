#include "ProcessSelector.h"

#include <tlhelp32.h>
#include <psapi.h>       // GetModuleFileNameEx, GetProcessMemoryInfo
#include <winternl.h>    // NT API 함수
#pragma comment(lib, "psapi.lib")

ProcessSelector::ProcessSelector()
{
}

ProcessSelector::~ProcessSelector()
{
}

void ProcessSelector::UpdateProcesses()
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

bool ProcessSelector::IsProcessRunning(const std::wstring& processName) const
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

bool ProcessSelector::IsProcessRunning(const DWORD processId) const
{
    auto it = processMap.find(processId);
    if (it != processMap.end())
    {
        return true;
    }
    
    return false;
}
