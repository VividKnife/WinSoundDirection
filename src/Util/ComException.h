#pragma once

#include <comdef.h>
#include <stdexcept>
#include <string>

namespace Util
{
class ComException : public std::runtime_error
{
public:
    explicit ComException(HRESULT hr, const char* expression)
        : std::runtime_error(BuildMessage(hr, expression))
        , m_hr(hr)
    {
    }

    [[nodiscard]] HRESULT Result() const noexcept { return m_hr; }

private:
    static std::string BuildMessage(HRESULT hr, const char* expression)
    {
        _com_error err(hr);
        std::wstring wide = err.ErrorMessage();
        std::string narrow(wide.begin(), wide.end());

        char buffer[32];
        sprintf_s(buffer, "0x%08X", static_cast<unsigned int>(hr));
        return std::string(expression) + " failed with " + buffer + ": " + narrow;
    }

    HRESULT m_hr;
};
}

#define THROW_IF_FAILED(hrcall)                                                                 \
    do                                                                                           \
    {                                                                                            \
        HRESULT _hr = (hrcall);                                                                  \
        if (FAILED(_hr))                                                                         \
        {                                                                                        \
            throw Util::ComException(_hr, #hrcall);                                              \
        }                                                                                        \
    } while (false)
