#include "FileSelector.h"

#include <vector>

#include "imgui_impl_dx12.h"

std::string FileSelector::OpenFileSelectDialog(const std::string& filterName, const std::vector<std::string>& filterTypes)
{
    OPENFILENAME ofn;
    WCHAR szFile[260];
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);

    // 필터 문자열 생성
    std::string filterStr;
    std::string extensionStr;
    
    if (!filterName.empty() && !filterTypes.empty()) {
        filterStr = filterName + "\0";
        for (size_t i = 0; i < filterTypes.size(); ++i) {
            if (i > 0) extensionStr += ";";
            extensionStr += filterTypes[i];
        }
        filterStr += extensionStr + "\0All Files (*.*)\0*.*\0\0";
    } else {
        filterStr = "All Files (*.*)\0*.*\0\0";
    }

    std::vector<wchar_t> wideFilter(filterStr.size() + 2);
    int len = MultiByteToWideChar(CP_UTF8, 0, filterStr.c_str(), 
                                filterStr.length() + 1,
                                wideFilter.data(), wideFilter.size());

    // 필터 설정
    ofn.lpstrFilter = wideFilter.data();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // 다이얼로그 표시
    if (GetOpenFileName(&ofn))
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8_str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, utf8_str.data(), size_needed, nullptr, nullptr);
        utf8_str.resize(size_needed - 1);
        return utf8_str;
    }

    return "";
}
