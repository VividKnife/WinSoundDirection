# Development Guide

## Cross-Platform Development

### Overview
This project is specifically designed for Windows and uses Windows-only APIs. However, you can still work on the codebase from macOS or Linux for development purposes.

## Platform Support

### ✅ Windows (Full Support)
- **Target Platform**: Windows 10/11 (64-bit)
- **Build System**: Visual Studio 2019+ or MSBuild
- **APIs Used**: WASAPI, Windows Spatial Audio, Direct2D, Win32
- **Functionality**: 100% - All features work

### ⚠️ macOS (Development Only)
- **Purpose**: Code editing, syntax checking, development
- **Build System**: CMake + Clang/GCC
- **APIs**: Mocked Windows APIs (headers only)
- **Functionality**: 0% - Compilation only, no runtime functionality

### ⚠️ Linux (Development Only)
- **Purpose**: Code editing, syntax checking, development  
- **Build System**: CMake + GCC/Clang
- **APIs**: Mocked Windows APIs (headers only)
- **Functionality**: 0% - Compilation only, no runtime functionality

## Building on Different Platforms

### Windows Development
```bash
# Visual Studio (Recommended)
1. Open SpatialAudioVisualizer.sln
2. Build -> Build Solution (Ctrl+Shift+B)

# Command Line
msbuild SpatialAudioVisualizer.sln /p:Configuration=Release /p:Platform=x64

# Batch Script
build.bat
```

### macOS Development
```bash
# Install dependencies
brew install cmake

# Build for syntax checking
./build_macos.sh

# Or manually
mkdir build_macos && cd build_macos
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(sysctl -n hw.ncpu)
```

### Linux Development
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake build-essential

# Build for syntax checking
mkdir build_linux && cd build_linux
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## Development Workflow

### 1. Code Editing
You can edit the source code on any platform using:
- **Visual Studio Code** (recommended)
- **CLion**
- **Qt Creator**
- **Vim/Neovim**
- **Any text editor**

### 2. Syntax Checking
On macOS/Linux, you can verify your code compiles:
```bash
# This will catch syntax errors, missing includes, etc.
cmake --build build_directory
```

### 3. Testing
- **Windows**: Full testing with `test_spatial_audio.bat`
- **macOS/Linux**: Code analysis only

### 4. Debugging
- **Windows**: Full debugging with Visual Studio debugger
- **macOS/Linux**: Static analysis, linting, code review

## Mock APIs

For cross-platform development, mock Windows APIs are provided in `mock/windows/`:

```cpp
// These headers provide type definitions but no functionality
#include "mock/windows/windows.h"    // Basic Windows types
#include "mock/windows/d2d1.h"       // Direct2D types
```

### What's Mocked
- ✅ **Type definitions**: HWND, DWORD, RECT, etc.
- ✅ **Constants**: VK_HOME, WM_PAINT, etc.
- ✅ **Structure definitions**: POINT, MSG, etc.
- ❌ **Function implementations**: All return dummy values
- ❌ **Runtime functionality**: Nothing actually works

## IDE Configuration

### Visual Studio Code
Create `.vscode/c_cpp_properties.json`:
```json
{
    "configurations": [
        {
            "name": "Windows",
            "includePath": [
                "${workspaceFolder}/src",
                "${workspaceFolder}/resources"
            ],
            "defines": [
                "_WIN32_WINNT=0x0A00",
                "WIN32_LEAN_AND_MEAN",
                "NOMINMAX"
            ],
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17"
        },
        {
            "name": "macOS",
            "includePath": [
                "${workspaceFolder}/src",
                "${workspaceFolder}/mock/windows"
            ],
            "defines": [
                "MOCK_WINDOWS_APIS=1"
            ],
            "compilerPath": "/usr/bin/clang++",
            "cStandard": "c17",
            "cppStandard": "c++17"
        }
    ]
}
```

### CLion
Configure CMake profiles:
- **Windows**: Use Visual Studio generator
- **macOS/Linux**: Use Unix Makefiles generator

## Limitations on Non-Windows Platforms

### What Won't Work
- ❌ Audio capture (no WASAPI)
- ❌ Spatial audio processing (no Windows Spatial Audio APIs)
- ❌ Window management (no Win32 APIs)
- ❌ System tray (no Shell32 APIs)
- ❌ Global hotkeys (no Windows message system)
- ❌ Direct2D rendering (no DirectX)

### What You Can Do
- ✅ Edit source code
- ✅ Check syntax and compilation
- ✅ Review code structure
- ✅ Plan architecture changes
- ✅ Write documentation
- ✅ Create tests (unit tests for cross-platform logic)

## Recommended Development Setup

### For Windows Developers
1. **Primary**: Visual Studio 2019+ on Windows
2. **Secondary**: Visual Studio Code for quick edits
3. **Testing**: Native Windows testing

### For macOS/Linux Developers
1. **Primary**: Visual Studio Code with C++ extension
2. **Build**: CMake for syntax checking
3. **Testing**: Windows VM or remote Windows machine
4. **Collaboration**: Git workflow with Windows developers

## Contributing

### Code Style
- Follow existing C++ style in the project
- Use consistent indentation (4 spaces)
- Add comments for complex logic
- Keep Windows-specific code clearly marked

### Pull Request Workflow
1. **Develop** on your preferred platform
2. **Test syntax** with CMake build
3. **Submit PR** with clear description
4. **Windows testing** will be done by maintainers

### Testing Requirements
- All PRs must be tested on Windows
- Include test cases for new features
- Update documentation for API changes

## Troubleshooting

### Common Issues on macOS/Linux

**Missing Windows headers:**
```bash
# Solution: Use mock headers
cmake .. -DMOCK_WINDOWS_APIS=1
```

**Undefined Windows functions:**
```cpp
// Solution: Add to mock headers or use preprocessor guards
#ifdef _WIN32
    // Windows-specific code
#else
    // Mock or alternative implementation
#endif
```

**Linking errors:**
```bash
# Expected on non-Windows platforms
# Focus on compilation success, not linking
```

### Getting Help
- **Windows issues**: Check Visual Studio documentation
- **Cross-platform issues**: Check CMake documentation
- **Project issues**: Create GitHub issue

## Future Improvements

### Potential Cross-Platform Features
- Abstract audio interface for different platforms
- Cross-platform window management layer
- Platform-specific implementations

### Current Limitations
- Tightly coupled to Windows APIs
- No abstraction layer for platform differences
- Windows-specific audio processing

This would require significant refactoring to support true cross-platform functionality.