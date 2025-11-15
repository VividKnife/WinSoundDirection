cmake_minimum_required(VERSION 3.16)
project(SpatialAudioVisualizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Building for Windows")
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Building for macOS - Note: This is a Windows-specific application")
    message(WARNING "This project requires Windows APIs and will not function on macOS")
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Building for Linux - Note: This is a Windows-specific application")
    message(WARNING "This project requires Windows APIs and will not function on Linux")
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_WIN32_WINNT=0x0A00)  # Windows 10
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
# Collect sources
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Platform-specific configurations
if(PLATFORM_WINDOWS)
    # Windows-specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d2d1
        dwrite
        winmm
        ole32
        shell32
        user32
        gdi32
        dwmapi
        comctl32
        comdlg32
        psapi
    )
    
    # Windows-specific compile definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
    
    # Set subsystem to Windows (GUI application)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    # Add resource file if building with MSVC
    if(MSVC)
        target_sources(${PROJECT_NAME} PRIVATE resources/resource.rc)
    endif()
    
elseif(PLATFORM_MACOS OR PLATFORM_LINUX)
    # For non-Windows platforms, we can only do syntax checking
    message(WARNING "Building on non-Windows platform - executable will not function")
    message(STATUS "This build is for syntax checking and development purposes only")
    
    # Mock Windows APIs for compilation (headers only)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        MOCK_WINDOWS_APIS=1
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # Add mock headers directory if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/mock/windows")
        target_include_directories(${PROJECT_NAME} PRIVATE mock/windows)
    endif()
endif()

# Compiler-specific settings
if(MSVC)
    # Enable parallel compilation
    target_compile_options(${PROJECT_NAME} PRIVATE /MP)
    
    # Set runtime library
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG _DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
endif()

# Installation (Windows only)
if(PLATFORM_WINDOWS)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
    
    install(FILES
        README.md
        USAGE.md
        DESTINATION .
    )
    
    install(DIRECTORY resources/
        DESTINATION resources
        FILES_MATCHING PATTERN "*.ico" PATTERN "*.md"
    )
endif()

# Custom targets
add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${PROJECT_NAME}"
)

# Testing
enable_testing()

# Add a simple test to verify the executable can be built
add_test(NAME build_test
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
)

# Print build information
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(PLATFORM_WINDOWS)
    message(STATUS "Target: Windows executable")
else()
    message(STATUS "Target: Syntax check only (Windows APIs required for functionality)")
endif()
message(STATUS "===========================")
message(STATUS "")