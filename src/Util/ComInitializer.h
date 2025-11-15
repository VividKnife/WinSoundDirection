#pragma once

#include <windows.h>

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
