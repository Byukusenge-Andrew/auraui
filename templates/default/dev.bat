@echo off
REM AuraUI Live Development Mode
auraui dev
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Development session ended with code %ERRORLEVEL%.
    pause
)
