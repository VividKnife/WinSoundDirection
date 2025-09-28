@echo off
echo Building Spatial Audio Visualizer...

REM 检查是否安装了Visual Studio
where msbuild >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: MSBuild not found. Please install Visual Studio or Visual Studio Build Tools.
    echo You can also run this from a Visual Studio Developer Command Prompt.
    pause
    exit /b 1
)

REM 创建输出目录
if not exist bin mkdir bin
if not exist obj mkdir obj

REM 检查资源文件
echo Checking resources...
if not exist resources\app_icon.ico (
    echo Creating application icon...
    python resources\create_better_icon.py
)

REM 构建Debug版本
echo Building Debug configuration...
msbuild SpatialAudioVisualizer.sln /p:Configuration=Debug /p:Platform=x64 /nologo /verbosity:minimal

if %ERRORLEVEL% NEQ 0 (
    echo Debug build failed!
    pause
    exit /b 1
)

REM 构建Release版本
echo Building Release configuration...
msbuild SpatialAudioVisualizer.sln /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal

if %ERRORLEVEL% NEQ 0 (
    echo Release build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Debug executable: bin\Debug\SpatialAudioVisualizer.exe
echo Release executable: bin\Release\SpatialAudioVisualizer.exe
echo.
pause