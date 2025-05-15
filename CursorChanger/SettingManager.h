#pragma once
#include <memory>
#include <string>

#include "CursorSetting.h"

class SettingManager
{
public:
    SettingManager(void);
    ~SettingManager(void);
    
    void CreateSettingsFile(const std::string& path) const;
    bool LoadSettingsFile(const std::string& path);
    bool UpdateSettingsFile(const std::string& path);
    std::string GetExecutablePath();
    std::string GetSettingsPath();

    static constexpr auto SETTINGS_FILE_NAME = "settings.ini";
    static constexpr auto SETTINGS_TYPE_CURSOR = "[Cursor]";
    static constexpr auto SETTINGS_SEPARATOR = "=";
    static constexpr auto SETTINGS_PATH_SEPARATOR= "\\";
    std::string settingsPath;
    
    std::shared_ptr<CursorSetting> pCursorSetting;
private:
    std::string SerializeSettings(const std::string& path) const;
};
