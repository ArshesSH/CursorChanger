#pragma once
#include <assert.h>
#include <string>
#include <unordered_map>

#include "CursorChanger.h"


class DynamicLibraryLoader
{
public:
    DynamicLibraryLoader();
    ~DynamicLibraryLoader();

    static bool LoadLibraryFrom(const std::wstring& libraryName);
    static bool UnloadLibrary(const std::wstring& libraryName);
    static void UnloadAllLibraries();
    template <typename T>
    static T GetFunctionOrNull(const std::wstring& libraryName, const std::string& functionName)
    {
        // 라이브러리 이름과 함수 이름을 모두 포함하는 고유한 키 생성
        std::string key = std::string(libraryName.begin(), libraryName.end()) + "::" + functionName;
        auto it = loadedFunctions.find(key);
        if (it != loadedFunctions.end())
        {
            return reinterpret_cast<T>(it->second);
        }

        if (!IsLibraryLoaded(libraryName))
        {
            if (!LoadLibraryFrom(libraryName))
            {
                assert(false && "Failed to load library");
                return nullptr;
            }
        }

        auto libIt = loadedModules.find(libraryName);
        if (libIt == loadedModules.end())
        {
            assert(false && "Failed to load library");
            return nullptr;
        }
    
        FARPROC func = GetProcAddress(libIt->second, functionName.c_str());
        if (func)
        {
            loadedFunctions[key] = func;
            return reinterpret_cast<T>(func);
        }

        assert(false && "Failed to load function");
        return nullptr;
    }

    
    static bool IsLibraryLoaded(const std::wstring& libraryName);
    
private:
    static std::unordered_map<std::wstring, HMODULE> loadedModules;
    static std::unordered_map<std::string, FARPROC> loadedFunctions;
};
