#pragma once
#include <string>

struct CursorSetting
{
public:
    CursorSetting()
        : shouldChangeByProcess(false), isFocusOnly(false)
    {
    }
    ~CursorSetting();

    static std::string Serialize(const CursorSetting& setting);
    static CursorSetting Deserialize(const std::string& data);

    std::string cursorPath;
    std::string targetProcess;
    bool shouldChangeByProcess;
    bool isFocusOnly;
};
