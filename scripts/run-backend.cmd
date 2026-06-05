@echo off
setlocal

title Run Backend

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BACKEND_DIR=%PROJECT_ROOT%\backend"
set "BACKEND_EXE=%BACKEND_DIR%\build\sports_equipment_backend.exe"
set "UCRT_BIN=C:\msys64\ucrt64\bin"

echo [INFO] Project root : %PROJECT_ROOT%
echo [INFO] Backend exe  : %BACKEND_EXE%

if not exist "%BACKEND_EXE%" (
  echo [ERROR] Backend executable not found.
  echo [TIP] Please run build-backend.cmd first.
  pause
  exit /b 1
)

if not exist "%UCRT_BIN%" (
  echo [ERROR] UCRT runtime/bin directory not found: %UCRT_BIN%
  pause
  exit /b 1
)

set "PATH=%UCRT_BIN%;%PATH%"
cd /d "%BACKEND_DIR%"

echo [INFO] Starting backend at http://127.0.0.1:8000
"%BACKEND_EXE%"

echo.
echo [INFO] Backend process exited.
pause
