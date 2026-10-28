@echo off
setlocal
echo ========================================================
echo   Running AuraUI Offline Test Suite
echo ========================================================

cd /d "%~dp0"

:: 1. Configure CMake with tests enabled if not already done
if not exist "build\CMakeCache.txt" (
    echo [1/3] Configuring CMake with tests enabled...
    cmake -B build -A x64 -DAURA_BUILD_TESTS=ON
    if %ERRORLEVEL% NEQ 0 (
        echo [Error] CMake configuration failed.
        exit /b %ERRORLEVEL%
    )
)

:: 2. Build the test executable
echo [2/3] Compiling test binary (auraui_tests)...
cmake --build build --config Release --target auraui_tests
if %ERRORLEVEL% NEQ 0 (
    echo [Error] Test compilation failed.
    exit /b %ERRORLEVEL%
)

:: 3. Run test runner
echo [3/3] Executing unit tests...
echo --------------------------------------------------------
if exist "build\tests\Release\auraui_tests.exe" (
    "build\tests\Release\auraui_tests.exe" %*
) else if exist "build\Release\auraui_tests.exe" (
    "build\Release\auraui_tests.exe" %*
) else (
    ctest --test-dir build -C Release --output-on-failure
)

echo --------------------------------------------------------
if %ERRORLEVEL% EQU 0 (
    echo [Success] All offline tests passed!
) else (
    echo [Failure] Tests failed. Check output above.
)
