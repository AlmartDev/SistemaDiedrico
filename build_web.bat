@echo off
call "C:\users\{yourusrname}\emsdk\emsdk_env.bat"
mkdir build_web 2>nul
cd build_web
cmake .. -DBUILD_WEB=ON
cmake --build .