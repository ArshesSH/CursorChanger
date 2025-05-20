#include "SystemTrayManager.h"
#include <codecvt>
#include <locale>

SystemTrayManager::SystemTrayManager(HWND hWnd, HICON hIcon, const std::string& tooltip)
    : hWnd(hWnd), hIcon(hIcon), tooltip(tooltip), isVisible(false)
{
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIcon;
    
    std::wstring wTooltip(tooltip.begin(), tooltip.end());
    wcsncpy_s(nid.szTip, wTooltip.c_str(), _TRUNCATE);
}

SystemTrayManager::~SystemTrayManager()
{
    RemoveFromTray();
}

bool SystemTrayManager::AddToTray()
{
    bool result = Shell_NotifyIcon(NIM_ADD, &nid);
    if (result) {
        isVisible = true;
    }
    return result;
}

bool SystemTrayManager::RemoveFromTray()
{
    if (isVisible) {
        bool result = Shell_NotifyIcon(NIM_DELETE, &nid);
        if (result) {
            isVisible = false;
        }
        return result;
    }
    return true;
}

bool SystemTrayManager::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TRAYICON && wParam == nid.uID) {
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
                if (onLeftClickCallback) onLeftClickCallback();
                return true;
            case WM_RBUTTONUP:
                if (onRightClickCallback) onRightClickCallback();
                ShowContextMenu();
                return true;
            case WM_LBUTTONDBLCLK:
                if (onDoubleClickCallback) onDoubleClickCallback();
                return true;
        }
    }
    return false;
}

void SystemTrayManager::ShowContextMenu()
{
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, L"Open");
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 2, L"Exit");
        
        SetMenuDefaultItem(hMenu, 1, FALSE);
        SetForegroundWindow(hWnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
        DestroyMenu(hMenu);
    }
}