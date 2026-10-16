@echo off
setlocal DisableDelayedExpansion
cd /d "%~dp0"
if not exist "modengine2_launcher.exe" (
    echo Extract this archive into your ModEngine2 directory, beside modengine2_launcher.exe.
    pause
    exit /b 1
)
if not exist "config_hair_gradient.toml" (
    echo Hair Gradient configuration is missing. Extract the complete archive again.
    pause
    exit /b 1
)
if not exist "mod\HairGradient\hair_gradient.dll" (
    echo Hair Gradient DLL is missing. Extract the complete archive again.
    pause
    exit /b 1
)
if not exist "mod\HairGradient\shaders\hair.hgz" (
    echo Hair Gradient shaders are missing. Extract the complete archive again.
    pause
    exit /b 1
)
if not exist "mod\HairGradient\shaders\hair_cloth.hgz" (
    echo Hair Gradient shaders are missing. Extract the complete archive again.
    pause
    exit /b 1
)
if exist "eldenring.exe" (
    "modengine2_launcher.exe" -t er -c "config_hair_gradient.toml" -p "eldenring.exe"
) else (
    "modengine2_launcher.exe" -t er -c "config_hair_gradient.toml"
)
if errorlevel 1 (
    echo ModEngine2 could not launch Elden Ring. Check its log and start Steam.
    pause
    exit /b 1
)
endlocal
