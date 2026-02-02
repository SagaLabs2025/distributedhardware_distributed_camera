@echo off
echo Checking development environment...

echo.
echo 1. Checking Visual Studio Compiler...
where cl >nul 2>&1
if %errorlevel% == 0 (
    echo    ✓ Visual Studio compiler found
    for /f "tokens=*" %%i in ('where cl') do echo    Location: %%i
) else (
    echo    ✗ Visual Studio compiler NOT found
    echo    Please install Visual Studio Build Tools
)

echo.
echo 2. Checking CMake...
cmake --version >nul 2>&1
if %errorlevel% == 0 (
    cmake --version | findstr "cmake version"
) else (
    echo    ✗ CMake NOT found
    echo    Please install CMake
)

echo.
echo 3. Checking Qt...
qmake --version >nul 2>&1
if %errorlevel% == 0 (
    qmake --version
) else (
    echo    ✗ Qt NOT found
    echo    Please install Qt and add to PATH
)

echo.
echo 4. Checking Qt Directory...
if exist "C:\Work\Qt\6.5.0\msvc2019_64" (
    echo    ✓ Qt directory found: C:\Work\Qt\6.5.0\msvc2019_64
) else (
    echo    ✗ Qt directory NOT found
    echo    Expected: C:\Work\Qt\6.5.0\msvc2019_64
)

echo.
echo Environment check completed.
pause