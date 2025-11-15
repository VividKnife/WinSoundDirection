#pragma once

#include <functional>

namespace Util
{
class ScopeExit
{
public:
    explicit ScopeExit(std::function<void()> callback)
        : m_callback(std::move(callback))
    {
    }

    ~ScopeExit()
    {
        if (m_callback)
        {
            m_callback();
        }
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit(ScopeExit&&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;

private:
    std::function<void()> m_callback;
};
}

#define SCOPE_EXIT(lambda) Util::ScopeExit scopeExit_##__LINE__(lambda)
