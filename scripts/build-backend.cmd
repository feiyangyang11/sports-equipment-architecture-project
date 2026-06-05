@echo off
setlocal

title Build Backend

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BACKEND_DIR=%PROJECT_ROOT%\backend"
set "BUILD_DIR=%BACKEND_DIR%\build"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
set "UCRT_BIN=C:\msys64\ucrt64\bin"

echo [INFO] Project root : %PROJECT_ROOT%
echo [INFO] Backend dir  : %BACKEND_DIR%

if not exist "%CMAKE_EXE%" (
  echo [ERROR] CMake not found: %CMAKE_EXE%
  pause
  exit /b 1
)

if not exist "%UCRT_BIN%" (
  echo [ERROR] UCRT runtime/bin directory not found: %UCRT_BIN%
  pause
  exit /b 1
)

set "PATH=%UCRT_BIN%;%PATH%"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
  echo [INFO] Build cache not found. Running CMake configure first...
  "%CMAKE_EXE%" -S "%BACKEND_DIR%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
  if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    pause
    exit /b 1
  )
)

echo [INFO] Building backend...
"%CMAKE_EXE%" --build "%BUILD_DIR%"
if errorlevel 1 (
  echo [ERROR] Backend build failed.
  pause
  exit /b 1
)

echo [OK] Backend build completed.
pause
