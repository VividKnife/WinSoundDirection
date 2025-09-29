#pragma once

// Windows compatibility header for compilation fixes
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <memory>
#include <string>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Manual implementations for older C++ standards
namespace compat {
    template<typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }

    template<typename T>
    constexpr const T& min(const T& a, const T& b) {
        return (a < b) ? a : b;
    }

    template<typename T>
    constexpr const T& max(const T& a, const T& b) {
        return (a > b) ? a : b;
    }
}

// Use compat namespace for missing functions
using compat::clamp;
using compat::min;
using compat::max;

// Suppress deprecation warnings for Windows functions
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// Suppress specific warnings
#pragma warning(push)
#pragma warning(disable: 4996) // 'localtime': This function or variable may be unsafe
#pragma warning(disable: 4819) // Unicode/encoding warnings

// At the end of files using this header, add: #pragma warning(pop)