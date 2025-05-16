#pragma once
#include <Windows.h>
#include <string>
#include <functional>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)

class SystemTrayManager
{
public:
    SystemTrayManager(HWND hWnd, HICON hIcon, const std::string& tooltip);
    ~SystemTrayManager();

    bool AddToTray();
    bool RemoveFromTray();
    bool IsVisible() const { return isVisible; }
    
    void SetOnLeftClickCallback(const std::function<void()>& callback) { onLeftClickCallback = callback; }
    void SetOnRightClickCallback(const std::function<void()>& callback) { onRightClickCallback = callback; }
    void SetOnDoubleClickCallback(const std::function<void()>& callback) { onDoubleClickCallback = callback; }
    
    bool ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ShowContextMenu();

private:
    HWND hWnd;
    HICON hIcon;
    std::string tooltip;
    NOTIFYICONDATA nid;
    bool isVisible;
    
    std::function<void()> onLeftClickCallback;
    std::function<void()> onRightClickCallback;
    std::function<void()> onDoubleClickCallback;
};