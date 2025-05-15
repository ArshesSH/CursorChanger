#pragma once
#include <functional>
#include <memory>

#include "CursorSetting.h"

class CursorSettingUI
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
    {}
    
    ~CursorSettingUI();
    
    void UpdateImGui();

private:
    std::shared_ptr<CursorSetting> pCursorSetting;
    
    // Interface
    std::function<void()> onClickSaveSetting;
    std::function<void()> onClickChangeCursor;
    std::function<void()> onClickRestoreCursor;
};
