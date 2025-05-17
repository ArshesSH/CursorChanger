#pragma once
#include <memory>
#include <string>

#include "CursorSetting.h"

struct SystemSetting;

class SettingManager
{
public:
    SettingManager();
    ~SettingManager();
    
    void CreateSettingsFile(const std::string& path) const;
    bool LoadSettingsFile(const std::string& path) const;
    bool UpdateSettingsFile(const std::string& path) const;
    std::string GetExecutablePath();
    std::string GetSettingsPath();

    static constexpr auto SETTINGS_FILE_NAME = "settings.ini";
    static constexpr auto SETTINGS_TYPE_CURSOR = "[Cursor]";
    static constexpr auto SETTINGS_TYPE_SYSTEM = "[System]";
    static constexpr auto SETTINGS_SEPARATOR = "=";
    static constexpr auto SETTINGS_PATH_SEPARATOR= "\\";
    
    std::string settingsPath;
    std::shared_ptr<CursorSetting> pCursorSetting;
    std::shared_ptr<SystemSetting> pSystemSetting;
private:
    std::string SerializeSettings(const std::string& path) const;
};
