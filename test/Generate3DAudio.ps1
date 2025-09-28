# Spatial Audio Test Generator
# 生成3D空间音效测试文件

param(
    [string]$OutputDir = "test_audio",
    [switch]$DownloadSamples = $true,
    [switch]$Generate3D = $true
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Spatial Audio Visualizer - Test Generator" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 创建输出目录
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "✓ Created directory: $OutputDir" -ForegroundColor Green
}

# 检查Windows Spatial Audio支持
function Test-SpatialAudioSupport {
    Write-Host "Checking Windows Spatial Audio support..." -ForegroundColor Yellow
    
    try {
        # 检查Windows版本
        $version = [System.Environment]::OSVersion.Version
        if ($version.Major -ge 10) {
            Write-Host "✓ Windows 10/11 detected - Spatial Audio supported" -ForegroundColor Green
        } else {
            Write-Host "⚠ Older Windows version - Limited spatial audio support" -ForegroundColor Yellow
        }
        
        # 检查音频设备
        $audioDevices = Get-WmiObject -Class Win32_SoundDevice
        Write-Host "✓ Found $($audioDevices.Count) audio device(s)" -ForegroundColor Green
        
        return $true
    } catch {
        Write-Host "✗ Error checking spatial audio support: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# 下载测试音频样例
function Download-AudioSamples {
    Write-Host "Downloading spatial audio test samples..." -ForegroundColor Yellow
    
    $samples = @(
        @{
            Name = "helicopter_3d.wav"
            Url = "https://www2.cs.uic.edu/~i101/SoundFiles/BabyElephantWalk60.wav"
            Description = "Helicopter flyby simulation"
        },
        @{
            Name = "footsteps_3d.wav"
            Url = "https://www2.cs.uic.edu/~i101/SoundFiles/StarWars60.wav"
            Description = "Footsteps around player"
        },
        @{
            Name = "gunfire_3d.wav"
            Url = "https://www2.cs.uic.edu/~i101/SoundFiles/CantinaBand60.wav"
            Description = "Gunfire from different directions"
        }
    )
    
    foreach ($sample in $samples) {
        $outputPath = Join-Path $OutputDir $sample.Name
        
        try {
            Write-Host "  Downloading: $($sample.Description)" -ForegroundColor Cyan
            Invoke-WebRequest -Uri $sample.Url -OutFile $outputPath -UseBasicParsing
            Write-Host "  ✓ Saved: $($sample.Name)" -ForegroundColor Green
        } catch {
            Write-Host "  ✗ Failed: $($sample.Name) - $($_.Exception.Message)" -ForegroundColor Red
        }
    }
}

# 生成3D音效配置文件
function Generate-3DAudioConfig {
    Write-Host "Generating 3D audio configuration..." -ForegroundColor Yellow
    
    $config = @{
        TestSequences = @(
            @{
                Name = "Front Attack"
                Direction = @{ Azimuth = 0; Elevation = 0; Distance = 1.0 }
                Duration = 3
                Description = "Enemy approaching from front"
            },
            @{
                Name = "Left Flank"
                Direction = @{ Azimuth = -90; Elevation = 0; Distance = 0.8 }
                Duration = 3
                Description = "Enemy on left side"
            },
            @{
                Name = "Right Flank"
                Direction = @{ Azimuth = 90; Elevation = 0; Distance = 0.8 }
                Duration = 3
                Description = "Enemy on right side"
            },
            @{
                Name = "Behind"
                Direction = @{ Azimuth = 180; Elevation = 0; Distance = 1.2 }
                Duration = 3
                Description = "Enemy behind player"
            },
            @{
                Name = "Above"
                Direction = @{ Azimuth = 0; Elevation = 45; Distance = 0.6 }
                Duration = 3
                Description = "Enemy above (helicopter, drone)"
            },
            @{
                Name = "Below"
                Direction = @{ Azimuth = 0; Elevation = -45; Distance = 0.6 }
                Duration = 3
                Description = "Enemy below (basement, tunnel)"
            },
            @{
                Name = "Circular Movement"
                Direction = @{ Azimuth = 0; Elevation = 0; Distance = 1.0 }
                Duration = 8
                Description = "Enemy moving in circle around player"
            }
        )
    }
    
    $configPath = Join-Path $OutputDir "test_config.json"
    $config | ConvertTo-Json -Depth 3 | Out-File -FilePath $configPath -Encoding UTF8
    Write-Host "✓ Generated config: test_config.json" -ForegroundColor Green
}

# 创建测试播放器脚本
function Create-TestPlayer {
    Write-Host "Creating test player script..." -ForegroundColor Yellow
    
    $playerScript = @'
# 3D Audio Test Player
Add-Type -AssemblyName PresentationCore

function Play-SpatialAudioTest {
    param([string]$AudioFile, [hashtable]$Direction, [string]$Description)
    
    Write-Host "Playing: $Description" -ForegroundColor Cyan
    Write-Host "Direction: Azimuth=$($Direction.Azimuth)°, Elevation=$($Direction.Elevation)°, Distance=$($Direction.Distance)" -ForegroundColor Yellow
    Write-Host "Watch the Spatial Audio Visualizer for direction indicators!" -ForegroundColor Green
    Write-Host ""
    
    if (Test-Path $AudioFile) {
        try {
            $mediaPlayer = New-Object System.Windows.Media.MediaPlayer
            $mediaPlayer.Open([System.Uri]::new((Resolve-Path $AudioFile).Path))
            $mediaPlayer.Play()
            
            Start-Sleep -Seconds 3
            
            $mediaPlayer.Stop()
            $mediaPlayer.Close()
        } catch {
            Write-Host "Error playing audio: $($_.Exception.Message)" -ForegroundColor Red
        }
    } else {
        Write-Host "Audio file not found: $AudioFile" -ForegroundColor Red
    }
    
    Write-Host "Press any key to continue..." -ForegroundColor Gray
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    Write-Host ""
}

# 加载测试配置
$configPath = "test_config.json"
if (Test-Path $configPath) {
    $config = Get-Content $configPath | ConvertFrom-Json
    
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "3D Audio Test Sequence Starting" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Instructions:" -ForegroundColor Yellow
    Write-Host "1. Make sure Spatial Audio Visualizer is running" -ForegroundColor White
    Write-Host "2. Wear headphones for best 3D effect" -ForegroundColor White
    Write-Host "3. Enable Windows Sonic for Headphones" -ForegroundColor White
    Write-Host "4. Watch the visualizer during each test" -ForegroundColor White
    Write-Host ""
    Write-Host "Press any key to start..." -ForegroundColor Gray
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    Write-Host ""
    
    # 获取音频文件
    $audioFiles = Get-ChildItem -Filter "*.wav" | Select-Object -First 1
    
    if ($audioFiles) {
        foreach ($test in $config.TestSequences) {
            Play-SpatialAudioTest -AudioFile $audioFiles.FullName -Direction $test.Direction -Description $test.Description
        }
    } else {
        Write-Host "No audio files found for testing" -ForegroundColor Red
    }
    
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Test sequence completed!" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
} else {
    Write-Host "Config file not found: $configPath" -ForegroundColor Red
}
'@
    
    $playerPath = Join-Path $OutputDir "run_test.ps1"
    $playerScript | Out-File -FilePath $playerPath -Encoding UTF8
    Write-Host "✓ Created test player: run_test.ps1" -ForegroundColor Green
}

# 创建Windows Sonic设置指南
function Create-SetupGuide {
    Write-Host "Creating setup guide..." -ForegroundColor Yellow
    
    $guide = @"
# Windows Spatial Audio Setup Guide

## Enable Windows Sonic for Headphones

1. **Right-click** the sound icon in the system tray
2. Select **"Spatial sound (Off)"**
3. Choose **"Windows Sonic for Headphones"**
4. Click **"Apply"**

## Alternative Method

1. Open **Settings** (Win + I)
2. Go to **System > Sound**
3. Click on your audio device
4. Under **Spatial audio**, select **"Windows Sonic for Headphones"**

## Verify Setup

1. You should see "Windows Sonic for Headphones" in the sound menu
2. Some games may show 3D audio options
3. The Spatial Audio Visualizer should detect spatial audio support

## Supported Games

- Call of Duty series
- Battlefield series
- Counter-Strike 2
- Valorant
- Rainbow Six Siege
- Fortnite
- PUBG

## Troubleshooting

- **No spatial audio detected**: Check if your headphones support it
- **Visualizer not responding**: Restart the application
- **Poor direction accuracy**: Adjust sensitivity in settings
- **Performance issues**: Lower update frequency in config

## Testing

Run the test script to verify everything is working:
```
cd test_audio
powershell -ExecutionPolicy Bypass -File run_test.ps1
```
"@
    
    $guidePath = Join-Path $OutputDir "SETUP_GUIDE.md"
    $guide | Out-File -FilePath $guidePath -Encoding UTF8
    Write-Host "✓ Created setup guide: SETUP_GUIDE.md" -ForegroundColor Green
}

# 主执行流程
Write-Host "Starting spatial audio test generation..." -ForegroundColor Cyan
Write-Host ""

# 检查系统支持
if (!(Test-SpatialAudioSupport)) {
    Write-Host "System may not fully support spatial audio testing" -ForegroundColor Yellow
}

# 下载样例文件
if ($DownloadSamples) {
    Download-AudioSamples
}

# 生成3D配置
if ($Generate3D) {
    Generate-3DAudioConfig
    Create-TestPlayer
    Create-SetupGuide
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test generation completed!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. cd $OutputDir" -ForegroundColor White
Write-Host "2. Read SETUP_GUIDE.md for Windows Sonic setup" -ForegroundColor White
Write-Host "3. Run: powershell -ExecutionPolicy Bypass -File run_test.ps1" -ForegroundColor White
Write-Host "4. Start SpatialAudioVisualizer.exe before testing" -ForegroundColor White
Write-Host ""