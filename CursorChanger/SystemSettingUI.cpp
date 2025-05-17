#include "SystemSettingUI.h"

#include "imgui.h"
#include "SystemSetting.h"

SystemSettingUI::~SystemSettingUI()
{
}

void SystemSettingUI::UpdateImGui()
{
    ImGui::SeparatorText("System Setting");
    ImGui::Checkbox("System tray mode", &pSystemSetting->isSystemTrayMode);
    ImGui::SetItemTooltip("Enable system tray mode when the application is minimized");
    ImGui::SameLine();
    ImGui::Checkbox("Auto start with Windows", &pSystemSetting->shouldRegisterStartUp);
    ImGui::SetItemTooltip("Apply when click save button");
}
