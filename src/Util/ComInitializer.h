#pragma once

#include <windows.h>
#include <objbase.h>

namespace Util
{
class ComInitializer
{
public:
    ComInitializer()
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }

    ~ComInitializer()
    {
        CoUninitialize();
    }
};
}
