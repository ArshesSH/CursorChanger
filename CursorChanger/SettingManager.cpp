#include "SettingManager.h"

#include <filesystem>
#include <fstream>

#include "imgui_impl_dx12.h"

SettingManager::SettingManager()
{
    pCursorSetting = std::make_shared<CursorSetting>();
    
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
}

// Create a new settings file by serializing the current settings
void SettingManager::CreateSettingsFile(const std::string& path) const
{
    std::ofstream settingsFile(path);
    const auto settingsData = SerializeSettings(path);
    settingsFile << settingsData;
    settingsFile.close();
}

bool SettingManager::LoadSettingsFile(const std::string& path)
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
    
    return true;
}

bool SettingManager::UpdateSettingsFile(const std::string& path)
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
    GetModuleFileNameW(NULL, path, MAX_PATH);
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
    return data;
}

