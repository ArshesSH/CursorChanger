#include "CursorSetting.h"

#include <sstream>

CursorSetting::~CursorSetting()
{
}

std::string CursorSetting::Serialize(const CursorSetting& setting)
{
    std::string result;
    result += "CursorPath=" + std::string(setting.cursorPath) + "\n";
    result += "TargetProcess=" + std::string(setting.targetProcess) + "\n";
    result += "ShouldChangeByProcess=" + std::to_string(setting.shouldChangeByProcess) + "\n";
    result += "IsFocusOnly=" + std::to_string(setting.isFocusOnly) + "\n";
    return result;
}

CursorSetting CursorSetting::Deserialize(const std::string& data)
{
    CursorSetting setting;
    std::string line;
    std::istringstream stream(data);
    
    while (std::getline(stream, line))
    {
        if (line.starts_with("CursorPath="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.cursorPath = line.substr(pos + 1);
        }
        else if (line.starts_with("TargetProcess="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.targetProcess = line.substr(pos + 1);
        }
        else if (line.starts_with("ShouldChangeByProcess="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.shouldChangeByProcess = (line.substr(pos + 1) == "1" || 
                                               line.substr(pos + 1) == "true");
            else
                setting.shouldChangeByProcess = false;
        }
        else if (line.starts_with("IsFocusOnly="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.isFocusOnly = (line.substr(pos + 1) == "1" || 
                                      line.substr(pos + 1) == "true");
            else
                setting.isFocusOnly = false;
        }
    }
    return setting;
}
