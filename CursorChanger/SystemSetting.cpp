#include "SystemSetting.h"

#include <sstream>

std::string SystemSetting::Serialize(const SystemSetting& setting)
{
    std::string result;
    result += "IsSystemTrayMode=" + std::to_string(setting.isSystemTrayMode) + "\n";
    result += "ShouldRegisterStartUp=" + std::to_string(setting.shouldRegisterStartUp) + "\n";
    return result;
}

SystemSetting SystemSetting::Deserialize(const std::string& data)
{
    SystemSetting setting;
    std::string line;
    std::istringstream stream(data);

    while (std::getline(stream, line))
    {
        if (line.starts_with("IsSystemTrayMode="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.isSystemTrayMode = (line.substr(pos + 1) == "1" ||
                                            line.substr(pos + 1) == "true");
            else
                setting.isSystemTrayMode = false;
        }
        else if (line.starts_with("ShouldRegisterStartUp="))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos && pos + 1 < line.length())
                setting.shouldRegisterStartUp = (line.substr(pos + 1) == "1" ||
                                                  line.substr(pos + 1) == "true");
            else
                setting.shouldRegisterStartUp = false;
        }
    }

    return setting;
}
