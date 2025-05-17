#pragma once
#include <string>

struct SystemSetting
{
    SystemSetting()
        :
    isSystemTrayMode(false),
    shouldRegisterStartUp(false)
    {
    }
    
    static std::string Serialize(const SystemSetting& setting);
    static SystemSetting Deserialize(const std::string& data);

    bool isSystemTrayMode;
    bool shouldRegisterStartUp;
};
