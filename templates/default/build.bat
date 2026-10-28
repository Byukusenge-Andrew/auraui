@echo off
REM AuraUI Standalone Release Build
auraui build
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [Error] Build failed with code %ERRORLEVEL%.
    pause
) else (
    echo.
    echo [Success] Executable created in dist\
    pause
)
