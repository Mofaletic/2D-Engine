@echo off
setlocal

set "VS_VCVARS=D:\VS 2022\VC\Auxiliary\Build\vcvars64.bat"
set "CMAKE_EXE=D:\VS 2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "PRESET=x64-debug"

if not exist "%VS_VCVARS%" (
  echo [ERROR] Cannot find vcvars64.bat: "%VS_VCVARS%"
  exit /b 1
)

if not exist "%CMAKE_EXE%" (
  echo [ERROR] Cannot find cmake.exe: "%CMAKE_EXE%"
  exit /b 1
)

call "%VS_VCVARS%"
if errorlevel 1 (
  echo [ERROR] Failed to initialize Visual Studio build environment.
  exit /b 1
)

echo [INFO] Configuring preset: %PRESET%
"%CMAKE_EXE%" --preset %PRESET%
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  exit /b 1
)

echo [INFO] Building directory: out/build/%PRESET%
"%CMAKE_EXE%" --build "out/build/%PRESET%"
if errorlevel 1 (
  echo [ERROR] CMake build failed.
  exit /b 1
)

echo [OK] Build completed successfully.
exit /b 0
