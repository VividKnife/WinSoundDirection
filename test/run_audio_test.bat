@echo off
title Spatial Audio Visualizer - Audio Test
color 0A

echo.
echo  ███████╗██████╗  █████╗ ████████╗██╗ █████╗ ██╗         
echo  ██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║         
echo  ███████╗██████╔╝███████║   ██║   ██║███████║██║         
echo  ╚════██║██╔═══╝ ██╔══██║   ██║   ██║██╔══██║██║         
echo  ███████║██║     ██║  ██║   ██║   ██║██║  ██║███████╗    
echo  ╚══════╝╚═╝     ╚═╝  ╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝    
echo.
echo   █████╗ ██╗   ██╗██████╗ ██╗ ██████╗     ████████╗███████╗███████╗████████╗
echo  ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗    ╚══██╔══╝██╔════╝██╔════╝╚══██╔══╝
echo  ███████║██║   ██║██║  ██║██║██║   ██║       ██║   █████╗  ███████╗   ██║   
echo  ██╔══██║██║   ██║██║  ██║██║██║   ██║       ██║   ██╔══╝  ╚════██║   ██║   
echo  ██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝       ██║   ███████╗███████║   ██║   
echo  ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝        ╚═╝   ╚══════╝╚══════╝   ╚═╝   
echo.
echo ================================================================================
echo                          3D Audio Testing Suite
echo ================================================================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [✓] Running with administrator privileges
) else (
    echo [!] Note: Some features may require administrator privileges
)

echo.
echo [1] Checking system requirements...

REM 检查PowerShell版本
for /f "tokens=*" %%i in ('powershell -Command "$PSVersionTable.PSVersion.Major"') do set PS_VERSION=%%i
echo [✓] PowerShell version: %PS_VERSION%

REM 检查Windows版本
for /f "tokens=4-5 delims=. " %%i in ('ver') do set WIN_VERSION=%%i.%%j
echo [✓] Windows version: %WIN_VERSION%

echo.
echo [2] Setting up test environment...

REM 设置执行策略
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force" >nul 2>&1
echo [✓] PowerShell execution policy configured

echo.
echo [3] Generating test files...
echo     This will download audio samples and create test configurations...
echo.

REM 运行PowerShell测试生成器
powershell -ExecutionPolicy Bypass -File "Generate3DAudio.ps1"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [✗] Test generation failed!
    echo     Please check your internet connection and try again.
    pause
    exit /b 1
)

echo.
echo [4] Test files ready!
echo.
echo ================================================================================
echo                              IMPORTANT SETUP
echo ================================================================================
echo.
echo Before running tests, please:
echo.
echo 1. Enable Windows Sonic for Headphones:
echo    • Right-click sound icon in system tray
echo    • Select "Spatial sound (Off)"
echo    • Choose "Windows Sonic for Headphones"
echo.
echo 2. Start the Spatial Audio Visualizer:
echo    • Run SpatialAudioVisualizer.exe
echo    • Position the window where you can see it
echo.
echo 3. Wear headphones for best 3D audio effect
echo.
echo ================================================================================
echo.

choice /C YN /M "Ready to run audio tests"
if errorlevel 2 goto :end

echo.
echo [5] Starting audio tests...
cd test_audio
powershell -ExecutionPolicy Bypass -File "run_test.ps1"

:end
echo.
echo ================================================================================
echo                              Test Complete
echo ================================================================================
echo.
echo If the visualizer correctly showed direction indicators during the tests,
echo your spatial audio setup is working properly!
echo.
echo For more information, check test_audio\SETUP_GUIDE.md
echo.
pause