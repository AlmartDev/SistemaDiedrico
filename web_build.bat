@echo off
setlocal

:: Set Emscripten path - UPDATE THIS TO YOUR ACTUAL PATH
set EMSDK_PATH=C:\Users\alons\emsdk

:: Clear build directory
if exist "build_web" rmdir /s /q "build_web"
mkdir "build_web"

:: Activate Emscripten environment
call "%EMSDK_PATH%\emsdk_env.bat" > nul 2>&1

:: Configure with Emscripten
cd build_web
echo Configuring with Emscripten...
emcmake cmake .. -DBUILD_WEB=ON -DCMAKE_BUILD_TYPE=Release -G "Ninja" || (
    echo Configuration failed
    pause
    exit /b 1
)

:: Build the project
echo Building project...
emmake ninja -v || (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
echo Output files:
dir /b *.html *.js *.wasm

pause