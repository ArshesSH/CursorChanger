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

    using GetForegroundWindowFunc = HWND(WINAPI*)();
    using GetWindowThreadProcessIdFunc = DWORD(WINAPI*)(HWND, LPDWORD);
    using OpenProcessFunc = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
    using QueryFullProcessImageNameWFunc = BOOL(WINAPI*)(HANDLE, DWORD, LPWSTR, LPDWORD);
    using CloseHandleFunc = BOOL(WINAPI*)(HANDLE);
    using CreateToolhelp32SnapshotFunc = HANDLE(WINAPI*)(DWORD, DWORD);

    static GetForegroundWindowFunc getForegroundWindowFunc;
    static GetWindowThreadProcessIdFunc getWindowThreadProcessIdFunc;
    static OpenProcessFunc openProcessFunc;
    static QueryFullProcessImageNameWFunc queryFullProcessImageNameWFunc;
    static CloseHandleFunc closeHandleFunc;
    static CreateToolhelp32SnapshotFunc createToolhelp32SnapshotFunc;
};
