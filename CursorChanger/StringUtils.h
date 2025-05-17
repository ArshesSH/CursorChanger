#pragma once
#include <string>
#include <Windows.h>

namespace StringUtils {
    inline std::wstring Utf8ToWide(const std::string& utf8Str) {
        if (utf8Str.empty()) return L"";
        
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        if (size <= 0) return L"";
        
        std::wstring wideStr(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], size);
        return wideStr;
    }
    
    inline std::string WideToUtf8(const std::wstring& wideStr) {
        if (wideStr.empty()) return "";
        
        int size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "";
        
        std::string utf8Str(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], size, nullptr, nullptr);
        return utf8Str;
    }
}