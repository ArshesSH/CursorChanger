#pragma once
#include <functional>
#include <memory>

#include "CursorSetting.h"
#include "ImGuiBaseUI.h"
#include "ProcessManager.h"
#include "SelectorUI.h"

class CursorSettingUI : public ImGuiBaseUI
{
public:
    CursorSettingUI(const std::shared_ptr<CursorSetting>& pCursorSetting, 
                    const std::function<void()>& onClickSaveSetting = nullptr,
                    const std::function<void()>& onClickChangeCursor = nullptr,
                    const std::function<void()>& onClickRestoreCursor = nullptr)
    :
        pCursorSetting(pCursorSetting),
        onClickSaveSetting(onClickSaveSetting),
        onClickChangeCursor(onClickChangeCursor),
        onClickRestoreCursor(onClickRestoreCursor)
    {
        if (processNameBuffer[0] == 0 && !pCursorSetting->targetProcess.empty()) {
            strcpy_s(processNameBuffer, pCursorSetting->targetProcess.c_str());
        }

        spaceSize = ImVec2(0, SPACE_SIZE);
    }
    
    ~CursorSettingUI() override;
    
    void UpdateImGui() override;

private:
    static constexpr float SPACE_SIZE = 30.0f;
    
    std::shared_ptr<CursorSetting> pCursorSetting;
    char processNameBuffer[256] = { 0 };
    ImVec2 spaceSize;
    // Interface
    std::function<void()> onClickSaveSetting;
    std::function<void()> onClickChangeCursor;
    std::function<void()> onClickRestoreCursor;
};
