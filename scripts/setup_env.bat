@echo off
echo Setting up Distributed Camera Test Environment...

REM 设置Qt环境变量
set QTDIR=C:\Work\Qt\6.5.0\msvc2019_64
set PATH=%QTDIR%\bin;%PATH%

REM 设置Visual Studio Build Tools路径
set VSBT_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools
set PATH=%VSBT_PATH%\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;%PATH%
set PATH=%VSBT_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%

REM 设置项目特定环境变量
set DCAMERA_COMM_MODE=tcp
set DCAMERA_LOG_LEVEL=INFO

echo.
echo Environment variables set:
echo QTDIR = %QTDIR%
echo DCAMERA_COMM_MODE = %DCAMERA_COMM_MODE%
echo DCAMERA_LOG_LEVEL = %DCAMERA_LOG_LEVEL%
echo.
echo Ready to build and run!
echo.