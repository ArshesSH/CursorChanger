#pragma once
#include <string>
#include <unordered_map>

#include "imgui_impl_dx12.h"

class ProcessSelector
{
public:
    ProcessSelector();
    ~ProcessSelector();

    void UpdateProcesses();
    const std::unordered_map<DWORD, std::wstring>& GetProcessList() const { return processMap; }
    bool IsProcessRunning(const std::wstring& processName) const;
    bool IsProcessRunning(const DWORD processId) const;

private:
    std::unordered_map<DWORD, std::wstring> processMap;
};
