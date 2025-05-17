#pragma once
#include <functional>
#include <memory>

#include "imgui.h"
#include "ImGuiBaseUI.h"

struct SystemSetting;

class SystemSettingUI : public ImGuiBaseUI
{
public:
    SystemSettingUI(const std::shared_ptr<SystemSetting>& pSystemSetting)
        :
    pSystemSetting(pSystemSetting)
    {
    }
    ~SystemSettingUI() override;

    void UpdateImGui() override;
    
private:
    std::shared_ptr<SystemSetting> pSystemSetting;
};
