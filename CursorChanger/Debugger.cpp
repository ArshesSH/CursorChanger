#include "Debugger.h"

#include <chrono>

#include "imgui.h"


std::string Debugger::sMessage;
Debugger::Type Debugger::sMessageType;
std::mutex Debugger::sMutex;

Debugger::Debugger()
{
}

Debugger::~Debugger()
{
}

void Debugger::Log(const std::string& message, Type type)
{
    std::lock_guard lock(sMutex);
    sMessage = GetTimeStr() + message;
    sMessageType = type;
}

std::string Debugger::GetTimeStr()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_now;
    if (localtime_s(&tm_now, &time_t_now) != 0)
    {
        return "";
    }
    
    std::stringstream ss;
    ss << std::put_time(&tm_now, "[%H:%M:%S] ");

    return ss.str();
}

void Debugger::UpdateImGui()
{
    std::string message;
    Type messageType;
    {
        std::lock_guard lock(sMutex);
        message = sMessage;
        messageType = sMessageType;
    }
    
    ImGui::SeparatorText("Debugger");
    switch (messageType)
    {
    case Type::Error:
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: ");
        ImGui::SameLine();
        ImGui::Text("%s", message.c_str());
        break;
    case Type::Warning:
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: ");
        ImGui::SameLine();
        ImGui::Text("%s", message.c_str());
        break;
    default: // Normal log
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Log: ");
        ImGui::SameLine();
        ImGui::Text("%s", message.c_str());
        break;
    }
}
