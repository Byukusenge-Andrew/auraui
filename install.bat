@echo off
setlocal
echo ========================================================
echo   AuraUI CLI - Add to User PATH Installer
echo ========================================================
echo.

set "BIN_DIR=%~dp0bin"
if not exist "%BIN_DIR%" (
    mkdir "%BIN_DIR%"
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$bin = (Resolve-Path '%BIN_DIR%').Path;" ^
    "$userPath = [Environment]::GetEnvironmentVariable('Path', 'User');" ^
    "if (-not $userPath) { $userPath = '' };" ^
    "$paths = $userPath -split ';' | Where-Object { $_ -ne '' };" ^
    "if ($paths -notcontains $bin) {" ^
    "    $newPath = ($paths + $bin) -join ';';" ^
    "    [Environment]::SetEnvironmentVariable('Path', $newPath, 'User');" ^
    "    Write-Host '[Success] Added ' -NoNewline -ForegroundColor Green;" ^
    "    Write-Host $bin -NoNewline -ForegroundColor White;" ^
    "    Write-Host ' to your User PATH!' -ForegroundColor Green;" ^
    "    Write-Host '';" ^
    "    Write-Host 'Open a new terminal and you can now run AuraUI from anywhere:' -ForegroundColor Cyan;" ^
    "    Write-Host '   auraui --help' -ForegroundColor Yellow;" ^
    "    Write-Host '   auraui new my-app' -ForegroundColor Yellow;" ^
    "} else {" ^
    "    Write-Host '[Info] ' -NoNewline -ForegroundColor Yellow;" ^
    "    Write-Host $bin -NoNewline -ForegroundColor White;" ^
    "    Write-Host ' is already in your PATH!' -ForegroundColor Yellow;" ^
    "}"

echo.
echo ========================================================
pause
