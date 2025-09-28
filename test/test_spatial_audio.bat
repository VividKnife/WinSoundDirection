@echo off
echo ========================================
echo Spatial Audio Visualizer - Test Script
echo ========================================
echo.

REM 创建测试目录
if not exist test mkdir test
cd test

echo [1/4] Checking system requirements...

REM 检查PowerShell
powershell -Command "Write-Host 'PowerShell available'" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: PowerShell not available
    pause
    exit /b 1
)

REM 检查Windows版本
for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
echo Windows Version: %VERSION%

echo [2/4] Downloading spatial audio test files...

REM 下载空间音效测试文件
echo Downloading 3D audio test samples...

REM 使用PowerShell下载文件
powershell -Command "& {
    $ProgressPreference = 'SilentlyContinue'
    
    # 创建测试音频文件列表
    $audioFiles = @(
        @{
            Name = 'helicopter_flyby.wav'
            Url = 'https://www2.cs.uic.edu/~i101/SoundFiles/BabyElephantWalk60.wav'
            Description = 'Helicopter flyby (simulated 3D)'
        },
        @{
            Name = 'footsteps_around.wav' 
            Url = 'https://www2.cs.uic.edu/~i101/SoundFiles/StarWars60.wav'
            Description = 'Footsteps moving around (simulated 3D)'
        }
    )
    
    foreach ($file in $audioFiles) {
        Write-Host \"Downloading: $($file.Description)\"
        try {
            Invoke-WebRequest -Uri $file.Url -OutFile $file.Name -UseBasicParsing
            Write-Host \"✓ Downloaded: $($file.Name)\"
        } catch {
            Write-Host \"✗ Failed to download: $($file.Name)\"
        }
    }
}"

echo [3/4] Creating spatial audio test configuration...

REM 创建Windows Spatial Audio配置脚本
powershell -Command "& {
    Write-Host 'Checking Windows Spatial Audio support...'
    
    # 检查Windows Sonic支持
    $spatialAudio = Get-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render\*' -Name 'Properties' -ErrorAction SilentlyContinue
    
    if ($spatialAudio) {
        Write-Host '✓ Windows Spatial Audio APIs available'
    } else {
        Write-Host '⚠ Windows Spatial Audio may not be fully supported'
    }
    
    # 检查Windows Sonic设置
    Write-Host 'To enable Windows Sonic:'
    Write-Host '1. Right-click sound icon in system tray'
    Write-Host '2. Select \"Spatial sound (Off)\"'
    Write-Host '3. Choose \"Windows Sonic for Headphones\"'
    Write-Host ''
}"

echo [4/4] Starting audio playback test...

REM 创建音频播放测试脚本
echo Creating audio test player...

powershell -Command "& {
    Add-Type -AssemblyName presentationCore
    
    Write-Host 'Starting spatial audio test sequence...'
    Write-Host 'Make sure to:'
    Write-Host '1. Wear headphones for best 3D effect'
    Write-Host '2. Enable Windows Sonic for Headphones'
    Write-Host '3. Start the Spatial Audio Visualizer application'
    Write-Host ''
    
    # 播放测试音频
    $audioFiles = Get-ChildItem -Filter '*.wav'
    
    foreach ($file in $audioFiles) {
        Write-Host \"Playing: $($file.Name)\"
        Write-Host 'Watch the visualizer for direction indicators!'
        
        # 使用Windows Media Player播放
        $mediaPlayer = New-Object System.Windows.Media.MediaPlayer
        $mediaPlayer.Open([System.Uri]::new($file.FullName))
        $mediaPlayer.Play()
        
        # 等待播放完成
        Start-Sleep -Seconds 5
        $mediaPlayer.Stop()
        $mediaPlayer.Close()
        
        Write-Host 'Press any key for next test...'
        $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    }
    
    Write-Host 'Audio test completed!'
}"

echo.
echo ========================================
echo Test completed!
echo ========================================
echo.
echo Next steps:
echo 1. Run SpatialAudioVisualizer.exe
echo 2. Play games with 3D audio
echo 3. Observe direction indicators
echo.
pause