#pragma once

// Suppress specific warnings for MSVC only
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // 'localtime': This function or variable may be unsafe
#pragma warning(disable : 4819) // Unicode/encoding warnings
#endif

// Suppress deprecation warnings for Windows functions
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// Prevent Windows.h from defining min/max macros that conflict with std::
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows compatibility header for compilation fixes
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <windows.h>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Provide clamp function for older C++ standards
#if __cplusplus < 201703L
template<typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}
#else
using std::clamp;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif