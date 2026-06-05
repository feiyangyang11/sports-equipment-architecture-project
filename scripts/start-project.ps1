param(
  [int]$BackendPort = 8000,
  [int]$FrontendPort = 5500,
  [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Escape-SingleQuotedString {
  param([string]$Value)
  return $Value -replace "'", "''"
}

function Find-PythonExecutable {
  $candidates = @(
    "D:\python.exe",
    "C:\msys64\ucrt64\bin\python.exe"
  )

  foreach ($candidate in $candidates) {
    if (Test-Path -LiteralPath $candidate) {
      return $candidate
    }
  }

  $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
  if ($pythonCommand) {
    return $pythonCommand.Source
  }

  throw "Python executable not found. Please install Python or update this script."
}

function Test-PortInUse {
  param([int]$Port)

  try {
    return $null -ne (Get-NetTCPConnection -LocalPort $Port -ErrorAction Stop |
      Select-Object -First 1)
  } catch {
    return $false
  }
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$backendDir = Join-Path $projectRoot "backend"
$frontendDir = Join-Path $projectRoot "frontend"
$backendExe = Join-Path $backendDir "build\sports_equipment_backend.exe"
$ucrtBin = "C:\msys64\ucrt64\bin"
$pythonExe = Find-PythonExecutable
$powershellExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"

if (-not (Test-Path -LiteralPath $backendExe)) {
  throw "Backend executable not found: $backendExe`nBuild the backend first, then run this script again."
}

if (Test-PortInUse -Port $BackendPort) {
  Write-Warning "Port $BackendPort is already in use. The backend may fail to start."
}

if (Test-PortInUse -Port $FrontendPort) {
  Write-Warning "Port $FrontendPort is already in use. The frontend server may fail to start."
}

$escapedBackendDir = Escape-SingleQuotedString $backendDir
$escapedFrontendDir = Escape-SingleQuotedString $frontendDir
$escapedBackendExe = Escape-SingleQuotedString $backendExe
$escapedPythonExe = Escape-SingleQuotedString $pythonExe
$escapedUcrtBin = Escape-SingleQuotedString $ucrtBin

$backendCommand = @"
Set-Location -LiteralPath '$escapedBackendDir'
`$env:PATH = '$escapedUcrtBin;' + `$env:PATH
& '$escapedBackendExe'
"@

$frontendCommand = @"
Set-Location -LiteralPath '$escapedFrontendDir'
& '$escapedPythonExe' -m http.server $FrontendPort -d '$escapedFrontendDir'
"@

if ($DryRun) {
  Write-Host "Project root : $projectRoot"
  Write-Host "Backend cmd  :" -ForegroundColor Cyan
  Write-Host $backendCommand
  Write-Host "Frontend cmd :" -ForegroundColor Cyan
  Write-Host $frontendCommand
  exit 0
}

Start-Process -FilePath $powershellExe `
  -WorkingDirectory $backendDir `
  -ArgumentList @("-NoExit", "-Command", $backendCommand)

Start-Sleep -Milliseconds 500

Start-Process -FilePath $powershellExe `
  -WorkingDirectory $frontendDir `
  -ArgumentList @("-NoExit", "-Command", $frontendCommand)

Write-Host "Backend  : http://127.0.0.1:$BackendPort" -ForegroundColor Green
Write-Host "Frontend : http://127.0.0.1:$FrontendPort/pages/login.html" -ForegroundColor Green
Write-Host "Two PowerShell windows were opened for the backend and frontend servers." -ForegroundColor Yellow
