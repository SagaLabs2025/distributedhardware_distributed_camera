@echo off
echo Downloading GN and Ninja build tools...

REM Create tools directory
mkdir tools
cd tools

REM Download Ninja (Windows x64)
echo Downloading Ninja...
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-win.zip' -OutFile 'ninja-win.zip'"

REM Extract Ninja
echo Extracting Ninja...
powershell -Command "Expand-Archive -Path 'ninja-win.zip' -DestinationPath '.'"

REM Download GN (Windows x64)
echo Downloading GN...
powershell -Command "Invoke-WebRequest -Uri 'https://chrome-infra-packages.appspot.com/dl/gn/gn/windows-amd64/+/latest' -OutFile 'gn.zip'"

REM Extract GN
echo Extracting GN...
powershell -Command "Expand-Archive -Path 'gn.zip' -DestinationPath '.'"

REM Add to PATH
echo Adding tools to PATH...
set PATH=%CD%;%PATH%

echo Tools downloaded and ready!
cd ..