#pragma once

// Suppress specific warnings first
#pragma warning(push)
#pragma warning(disable: 4996) // 'localtime': This function or variable may be unsafe
#pragma warning(disable: 4819) // Unicode/encoding warnings

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

// At the end of files using this header, add: #pragma warning(pop)