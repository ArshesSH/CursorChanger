#include "SettingManager.h"

#include <filesystem>
#include <fstream>

#include "Debugger.h"
#include "imgui_impl_dx12.h"
#include "SystemSetting.h"

SettingManager::SettingManager()
{
    pCursorSetting = std::make_shared<CursorSetting>();
    pSystemSetting = std::make_shared<SystemSetting>();
    
    // Check if the settings file exists
    settingsPath = GetSettingsPath();

    bool isFailed = true;
    if (std::filesystem::exists(settingsPath))
    {
        isFailed = !LoadSettingsFile(settingsPath);
    }

    if (isFailed)
    {
        CreateSettingsFile(settingsPath);
    }
}

SettingManager::~SettingManager()
{
    if (pCursorSetting != nullptr)
    {
        pCursorSetting.reset();
    }
    
    if (pSystemSetting != nullptr)
    {
        pSystemSetting.reset();
    }
}

// Create a new settings file by serializing the current settings
void SettingManager::CreateSettingsFile(const std::string& path) const
{
    std::ofstream settingsFile(path);
    const auto settingsData = SerializeSettings(path);
    settingsFile << settingsData;
    settingsFile.close();
}

bool SettingManager::LoadSettingsFile(const std::string& path) const
{
    std::ifstream settingsFile(path);
    if (settingsFile.is_open() == false)
    {
        return false;
    }

    std::string allData;
    std::string line;
    while (std::getline(settingsFile, line))
    {
        allData += line + "\n";
    }
    settingsFile.close();

    *pCursorSetting = CursorSetting::Deserialize(allData);
    *pSystemSetting = SystemSetting::Deserialize(allData);
    
    return true;
}

bool SettingManager::UpdateSettingsFile(const std::string& path) const
{
    std::fstream settingsFile(path);
    if (settingsFile.is_open() == false)
    {
        return false;
    }
    
    settingsFile << SerializeSettings(path);
    settingsFile.close();
    return true;
}

std::string SettingManager::GetExecutablePath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().string();
}

std::string SettingManager::GetSettingsPath()
{
    std::string exePath = GetExecutablePath();
    return exePath + SETTINGS_PATH_SEPARATOR + SETTINGS_FILE_NAME;
}

std::string SettingManager::SerializeSettings(const std::string& path) const
{
    std::string data;
    data += SETTINGS_TYPE_CURSOR;
    data += "\n";
    data += CursorSetting::Serialize(*pCursorSetting);
    data += "\n";
    data += SETTINGS_TYPE_SYSTEM;
    data += "\n";
    data += SystemSetting::Serialize(*pSystemSetting);
    return data;
}

