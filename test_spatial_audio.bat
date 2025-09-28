@echo off
setlocal enabledelayedexpansion
title Spatial Audio Visualizer - Complete Test Suite
color 0B

echo.
echo ████████████████████████████████████████████████████████████████████████████████
echo █                                                                              █
echo █    ███████╗██████╗  █████╗ ████████╗██╗ █████╗ ██╗                          █
echo █    ██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║                          █
echo █    ███████╗██████╔╝███████║   ██║   ██║███████║██║                          █
echo █    ╚════██║██╔═══╝ ██╔══██║   ██║   ██║██╔══██║██║                          █
echo █    ███████║██║     ██║  ██║   ██║   ██║██║  ██║███████╗                     █
echo █    ╚══════╝╚═╝     ╚═╝  ╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝                     █
echo █                                                                              █
echo █     █████╗ ██╗   ██╗██████╗ ██╗ ██████╗     ████████╗███████╗███████╗████████╗
echo █    ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗    ╚══██╔══╝██╔════╝██╔════╝╚══██╔══╝
echo █    ███████║██║   ██║██║  ██║██║██║   ██║       ██║   █████╗  ███████╗   ██║   █
echo █    ██╔══██║██║   ██║██║  ██║██║██║   ██║       ██║   ██╔══╝  ╚════██║   ██║   █
echo █    ██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝       ██║   ███████╗███████║   ██║   █
echo █    ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝        ╚═╝   ╚══════╝╚══════╝   ╚═╝   █
echo █                                                                              █
echo ████████████████████████████████████████████████████████████████████████████████
echo.
echo                           Complete Testing Suite
echo                        for Spatial Audio Visualizer
echo.
echo ================================================================================

REM 检查是否以管理员身份运行
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [✓] Administrator privileges detected
) else (
    echo [!] Running without administrator privileges
    echo     Some features may be limited
)

echo.
echo [STEP 1] System Requirements Check
echo ================================================================================

REM 检查Windows版本
for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
echo Windows Version: %VERSION%

if %VERSION% LSS 10.0 (
    echo [!] WARNING: Windows 10 or later recommended for best spatial audio support
) else (
    echo [✓] Windows version supports spatial audio
)

REM 检查PowerShell
powershell -Command "Write-Host 'PowerShell available'" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [✗] PowerShell not available - some tests will be skipped
    set POWERSHELL_AVAILABLE=0
) else (
    echo [✓] PowerShell available
    set POWERSHELL_AVAILABLE=1
)

REM 检查Python
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Python not found - will use alternative audio generation
    set PYTHON_AVAILABLE=0
) else (
    echo [✓] Python available for audio generation
    set PYTHON_AVAILABLE=1
)

echo.
echo [STEP 2] Test Environment Setup
echo ================================================================================

REM 创建测试目录
if not exist test mkdir test
echo [✓] Test directory ready

REM 设置PowerShell执行策略
if %POWERSHELL_AVAILABLE%==1 (
    powershell -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force" >nul 2>&1
    echo [✓] PowerShell execution policy configured
)

echo.
echo [STEP 3] Audio Test File Generation
echo ================================================================================

REM 生成测试音频文件
if %PYTHON_AVAILABLE%==1 (
    echo Generating synthetic 3D audio test files...
    python test/generate_test_audio.py
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Python audio generation failed, trying alternative method
        goto :alternative_audio
    ) else (
        echo [✓] Synthetic audio files generated successfully
        set AUDIO_FILES_READY=1
    )
) else (
    :alternative_audio
    echo Using alternative audio generation method...
    if %POWERSHELL_AVAILABLE%==1 (
        cd test
        powershell -ExecutionPolicy Bypass -File "Generate3DAudio.ps1"
        cd ..
        if %ERRORLEVEL% NEQ 0 (
            echo [!] Audio generation failed
            set AUDIO_FILES_READY=0
        ) else (
            echo [✓] Audio files ready
            set AUDIO_FILES_READY=1
        )
    ) else (
        echo [!] Cannot generate audio files - PowerShell not available
        set AUDIO_FILES_READY=0
    )
)

echo.
echo [STEP 4] Pre-Test Configuration
echo ================================================================================
echo.
echo IMPORTANT: Before running tests, please complete the following setup:
echo.
echo 1. ENABLE WINDOWS SONIC FOR HEADPHONES:
echo    ┌─────────────────────────────────────────────────────────────┐
echo    │ • Right-click the sound icon in your system tray           │
echo    │ • Select "Spatial sound (Off)"                             │
echo    │ • Choose "Windows Sonic for Headphones"                    │
echo    │ • You should see "Windows Sonic for Headphones" selected   │
echo    └─────────────────────────────────────────────────────────────┘
echo.
echo 2. START THE SPATIAL AUDIO VISUALIZER:
echo    ┌─────────────────────────────────────────────────────────────┐
echo    │ • Run SpatialAudioVisualizer.exe                           │
echo    │ • Position the window where you can clearly see it          │
echo    │ • The visualizer should show a circular interface           │
echo    └─────────────────────────────────────────────────────────────┘
echo.
echo 3. PREPARE YOUR AUDIO SETUP:
echo    ┌─────────────────────────────────────────────────────────────┐
echo    │ • Wear headphones (required for 3D audio effect)           │
echo    │ • Set volume to comfortable level                           │
echo    │ • Close other audio applications                            │
echo    └─────────────────────────────────────────────────────────────┘
echo.

choice /C YN /M "Have you completed the setup above? Ready to start tests"
if errorlevel 2 goto :skip_tests

echo.
echo [STEP 5] Running Audio Tests
echo ================================================================================

if %AUDIO_FILES_READY%==1 (
    echo Starting comprehensive audio test sequence...
    echo.
    echo INSTRUCTIONS DURING TESTS:
    echo • Watch the Spatial Audio Visualizer window
    echo • Listen for audio from different directions
    echo • Verify that visual indicators match audio direction
    echo • Each test will announce its expected direction
    echo.
    
    if %PYTHON_AVAILABLE%==1 (
        echo Running Python-generated audio tests...
        if exist "test_audio\PLAYLIST.txt" (
            echo Test files available:
            type "test_audio\PLAYLIST.txt"
            echo.
            echo Press any key to start playing test files...
            pause >nul
            
            REM 播放测试文件
            for %%f in (test_audio\test_*.wav) do (
                echo.
                echo Playing: %%f
                echo Watch the visualizer for direction indicators!
                start /wait "" "%%f"
                timeout /t 3 >nul
            )
        )
    ) else if %POWERSHELL_AVAILABLE%==1 (
        echo Running PowerShell audio tests...
        cd test\test_audio
        if exist "run_test.ps1" (
            powershell -ExecutionPolicy Bypass -File "run_test.ps1"
        )
        cd ..\..
    )
) else (
    echo [!] No audio test files available
    echo     You can manually test by playing games with 3D audio
)

echo.
echo [STEP 6] Manual Testing Guide
echo ================================================================================
echo.
echo If automated tests are not available, you can test manually:
echo.
echo RECOMMENDED GAMES FOR TESTING:
echo • Counter-Strike 2 (excellent 3D audio)
echo • Call of Duty series
echo • Valorant
echo • Battlefield series
echo • Rainbow Six Siege
echo.
echo WHAT TO LOOK FOR:
echo • Footsteps from different directions should trigger corresponding indicators
echo • Gunfire should show direction of shooter
echo • Explosions should indicate blast direction
echo • Voice chat should show speaker direction
echo.
echo TROUBLESHOOTING:
echo • No indicators: Check if Windows Sonic is enabled
echo • Wrong directions: Verify headphone left/right orientation
echo • Poor accuracy: Adjust sensitivity in visualizer settings
echo • Performance issues: Close other applications
echo.

:skip_tests
echo.
echo [STEP 7] Test Results Verification
echo ================================================================================
echo.
echo Please verify the following worked correctly:
echo.
echo ┌─ FRONT AUDIO ────────────────────────────────────────────────────────────┐
echo │ Expected: Indicator at TOP of visualizer (12 o'clock position)          │
echo │ Result: [ ] Correct  [ ] Incorrect  [ ] Not tested                      │
echo └──────────────────────────────────────────────────────────────────────────┘
echo.
echo ┌─ BACK AUDIO ─────────────────────────────────────────────────────────────┐
echo │ Expected: Indicator at BOTTOM of visualizer (6 o'clock position)        │
echo │ Result: [ ] Correct  [ ] Incorrect  [ ] Not tested                      │
echo └──────────────────────────────────────────────────────────────────────────┘
echo.
echo ┌─ LEFT AUDIO ─────────────────────────────────────────────────────────────┐
echo │ Expected: Indicator at LEFT of visualizer (9 o'clock position)          │
echo │ Result: [ ] Correct  [ ] Incorrect  [ ] Not tested                      │
echo └──────────────────────────────────────────────────────────────────────────┘
echo.
echo ┌─ RIGHT AUDIO ────────────────────────────────────────────────────────────┐
echo │ Expected: Indicator at RIGHT of visualizer (3 o'clock position)         │
echo │ Result: [ ] Correct  [ ] Incorrect  [ ] Not tested                      │
echo └──────────────────────────────────────────────────────────────────────────┘
echo.

choice /C YN /M "Did the visualizer correctly show direction indicators"

if errorlevel 2 (
    echo.
    echo TROUBLESHOOTING STEPS:
    echo 1. Verify Windows Sonic is enabled in sound settings
    echo 2. Check that headphones are worn correctly (L/R)
    echo 3. Restart the Spatial Audio Visualizer application
    echo 4. Try adjusting audio sensitivity in settings
    echo 5. Test with a known 3D audio game
    echo.
) else (
    echo.
    echo ✓ CONGRATULATIONS! Your spatial audio setup is working correctly!
    echo.
    echo The Spatial Audio Visualizer is ready for gaming. It will now:
    echo • Show real-time direction indicators for game audio
    echo • Help you locate enemies by sound
    echo • Work with any game that supports 3D/spatial audio
    echo • Remain visible even in fullscreen games
    echo.
)

echo.
echo ================================================================================
echo                              TEST COMPLETE
echo ================================================================================
echo.
echo Summary:
echo • System compatibility: Checked
echo • Audio files: %AUDIO_FILES_READY% (1=Ready, 0=Not available)
echo • Windows Sonic: User configured
echo • Visualizer: User tested
echo.
echo For ongoing use:
echo • Keep Windows Sonic enabled
echo • Run SpatialAudioVisualizer.exe before gaming
echo • Use Home key to toggle visibility
echo • Right-click tray icon for options
echo.
echo Thank you for testing the Spatial Audio Visualizer!
echo.
pause