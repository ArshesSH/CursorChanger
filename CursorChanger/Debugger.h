#pragma once
#include <mutex>
#include <string>

#include "ImGuiBaseUI.h"

class Debugger final : public ImGuiBaseUI
{
public:
    enum class Type
    {
        Log,
        Warning,
        Error,
    };

    Debugger();
    ~Debugger() override;

    static void Log(const std::string& message, Type type = Type::Log);
    static std::string GetTimeStr();
    void UpdateImGui() override;

public:
    static std::string sMessage;
    static Type sMessageType;
    static std::mutex sMutex;
    
};
