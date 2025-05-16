#pragma once
#include <string>
#include <unordered_map>

#include "imgui_impl_dx12.h"

class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

    static DWORD GetForegroundProcessIdOrZero();
    static std::string GetForegroundProcessNameOrEmpty();
    
    void UpdateProcesses();
    const std::unordered_map<DWORD, std::wstring>& GetProcessList() const { return processMap; }
    bool IsProcessRunning(const std::wstring& processName) const;
    bool IsProcessRunning(const std::string& processName) const;
    bool IsProcessRunning(DWORD processId) const;

private:
    std::unordered_map<DWORD, std::wstring> processMap;
};
