#!/bin/bash

echo "=========================================="
echo "Spatial Audio Visualizer - macOS Build"
echo "=========================================="
echo ""
echo "⚠️  WARNING: This is a Windows-specific application!"
echo ""
echo "This build script is provided for:"
echo "• Syntax checking and code validation"
echo "• Development environment setup"
echo "• Cross-platform code analysis"
echo ""
echo "The resulting executable will NOT function on macOS"
echo "as it requires Windows-specific APIs that don't exist"
echo "on macOS (WASAPI, Direct2D, Win32, etc.)"
echo ""
echo "=========================================="
echo ""

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake:"
    echo "   brew install cmake"
    exit 1
fi

# Check if a C++ compiler is available
if ! command -v clang++ &> /dev/null && ! command -v g++ &> /dev/null; then
    echo "❌ No C++ compiler found. Please install Xcode Command Line Tools:"
    echo "   xcode-select --install"
    exit 1
fi

echo "✅ CMake found: $(cmake --version | head -n1)"
if command -v clang++ &> /dev/null; then
    echo "✅ Clang++ found: $(clang++ --version | head -n1)"
elif command -v g++ &> /dev/null; then
    echo "✅ G++ found: $(g++ --version | head -n1)"
fi
echo ""

# Create build directory
BUILD_DIR="build_macos"
if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🔧 Configuring CMake project..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

echo ""
echo "🔨 Building project (syntax check only)..."
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build completed successfully!"
    echo ""
    echo "📝 Build Summary:"
    echo "   • Syntax checking: PASSED"
    echo "   • Code compilation: PASSED"
    echo "   • Executable created: YES (non-functional)"
    echo ""
    echo "⚠️  Remember: This executable requires Windows to run!"
    echo ""
    echo "To build a functional version:"
    echo "1. Use a Windows machine or VM"
    echo "2. Install Visual Studio 2019+"
    echo "3. Run build.bat"
    echo ""
    echo "Or use one of these alternatives:"
    echo "• GitHub Codespaces with Windows"
    echo "• Azure DevTest Labs"
    echo "• Parallels Desktop / VMware Fusion"
    echo ""
else
    echo ""
    echo "❌ Build failed!"
    echo ""
    echo "This is expected since the code uses Windows-specific APIs."
    echo "Common issues on macOS:"
    echo "• Missing Windows headers (windows.h, d2d1.h, etc.)"
    echo "• Undefined Windows functions"
    echo "• Missing Windows libraries"
    echo ""
    echo "To fix compilation errors for development:"
    echo "1. Create mock Windows headers"
    echo "2. Use preprocessor directives to exclude Windows code"
    echo "3. Focus on cross-platform logic only"
    echo ""
fi

echo "=========================================="