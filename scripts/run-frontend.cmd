@echo off
setlocal

title Run Frontend

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "FRONTEND_DIR=%PROJECT_ROOT%\frontend"
set "PYTHON_EXE="

if exist "D:\python.exe" set "PYTHON_EXE=D:\python.exe"

if not defined PYTHON_EXE (
  for /f "delims=" %%I in ('where python 2^>nul') do (
    if not defined PYTHON_EXE set "PYTHON_EXE=%%I"
  )
)

echo [INFO] Project root  : %PROJECT_ROOT%
echo [INFO] Frontend dir  : %FRONTEND_DIR%

if not defined PYTHON_EXE (
  echo [ERROR] Python executable not found.
  echo [TIP] Please install Python or update this script.
  pause
  exit /b 1
)

cd /d "%FRONTEND_DIR%"

echo [INFO] Starting frontend at http://127.0.0.1:5500/pages/login.html
"%PYTHON_EXE%" -m http.server 5500 -d "%FRONTEND_DIR%"

echo.
echo [INFO] Frontend server exited.
pause
