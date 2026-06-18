param(
  [string]$BaseUrl = "http://127.0.0.1:8000",
  [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$resultsDir = Join-Path $PSScriptRoot "results"
$backendExe = Join-Path $projectRoot "backend\build\sports_equipment_backend.exe"
$buildDir = Join-Path $projectRoot "backend\build"
$cmakeExe = "C:\Program Files\CMake\bin\cmake.exe"
$ucrtBin = "C:\msys64\ucrt64\bin"
$backendStdout = Join-Path $resultsDir "backend.stdout.log"
$backendStderr = Join-Path $resultsDir "backend.stderr.log"
$backendRestartStdout = Join-Path $resultsDir "backend-restart.stdout.log"
$backendRestartStderr = Join-Path $resultsDir "backend-restart.stderr.log"

New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

if (Get-NetTCPConnection -LocalPort 8000 -State Listen -ErrorAction SilentlyContinue) {
  throw "Port 8000 is already in use. Stop the existing backend before running T6."
}

if (-not $SkipBuild) {
  if (-not (Test-Path -LiteralPath $cmakeExe)) {
    throw "CMake was not found: $cmakeExe"
  }
  & $cmakeExe --build $buildDir
  if ($LASTEXITCODE -ne 0) {
    throw "Backend build failed."
  }
}

$environment = & (Join-Path $PSScriptRoot "setup-test-environment.ps1") -ProjectRoot $projectRoot
if (-not (Test-Path -LiteralPath $backendExe)) {
  throw "Backend executable was not found: $backendExe"
}

$oldPath = $env:PATH
$backendProcess = $null
$startupSeconds = 0
$functionalExit = -1
$performanceExit = -1
$concurrencyExit = -1
$frontendExit = -1
$recoveryExit = -1

try {
  $env:PATH = "$ucrtBin;$oldPath"
  $startupWatch = [System.Diagnostics.Stopwatch]::StartNew()
  $backendProcess = Start-Process `
    -FilePath $backendExe `
    -WorkingDirectory $environment.RuntimeBackend `
    -WindowStyle Hidden `
    -RedirectStandardOutput $backendStdout `
    -RedirectStandardError $backendStderr `
    -PassThru

  $healthy = $false
  for ($attempt = 0; $attempt -lt 60; $attempt++) {
    if ($backendProcess.HasExited) {
      throw "Backend exited before becoming healthy. See tests/results/backend.stderr.log."
    }
    try {
      $response = Invoke-WebRequest -Uri "$BaseUrl/health" -UseBasicParsing -TimeoutSec 2
      if ($response.StatusCode -eq 200) {
        $healthy = $true
        break
      }
    } catch {
      Start-Sleep -Milliseconds 250
    }
  }

  $startupWatch.Stop()
  $startupSeconds = [math]::Round($startupWatch.Elapsed.TotalSeconds, 3)
  if (-not $healthy) {
    throw "Backend did not become healthy within 15 seconds."
  }

  [pscustomobject]@{
    measuredAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
    startupSeconds = $startupSeconds
    thresholdSeconds = 300
    passed = $startupSeconds -le 300
  } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $resultsDir "startup-result.json") -Encoding UTF8

  & (Join-Path $PSScriptRoot "functional\run-functional-tests.ps1") `
    -BaseUrl $BaseUrl `
    -OutputPath (Join-Path $resultsDir "functional-results.json")
  $functionalExit = $LASTEXITCODE

  & python (Join-Path $PSScriptRoot "nonfunctional\performance_test.py") `
    --base-url $BaseUrl `
    --requests 200 `
    --concurrency 50 `
    --output (Join-Path $resultsDir "performance-result.json")
  $performanceExit = $LASTEXITCODE

  & python (Join-Path $PSScriptRoot "nonfunctional\concurrency_test.py") `
    --base-url $BaseUrl `
    --requests 20 `
    --output (Join-Path $resultsDir "concurrency-result.json")
  $concurrencyExit = $LASTEXITCODE

  & (Join-Path $PSScriptRoot "nonfunctional\frontend_smoke_test.ps1") `
    -FrontendDir (Join-Path $projectRoot "frontend") `
    -OutputPath (Join-Path $resultsDir "frontend-result.json")
  $frontendExit = $LASTEXITCODE

  $loginBody = @{
    username = "admin01"
    password = "Admin@123"
  } | ConvertTo-Json -Compress
  $loginBeforeRestart = Invoke-RestMethod `
    -Uri "$BaseUrl/api/login" `
    -Method Post `
    -ContentType "application/json" `
    -Body $loginBody
  $oldToken = [string]$loginBeforeRestart.token

  Stop-Process -Id $backendProcess.Id -Force
  $backendProcess.WaitForExit()
  $backendProcess = $null

  $restartWatch = [System.Diagnostics.Stopwatch]::StartNew()
  $backendProcess = Start-Process `
    -FilePath $backendExe `
    -WorkingDirectory $environment.RuntimeBackend `
    -WindowStyle Hidden `
    -RedirectStandardOutput $backendRestartStdout `
    -RedirectStandardError $backendRestartStderr `
    -PassThru

  $restartHealthy = $false
  for ($attempt = 0; $attempt -lt 60; $attempt++) {
    if ($backendProcess.HasExited) {
      throw "Backend exited during recovery test."
    }
    try {
      $response = Invoke-WebRequest -Uri "$BaseUrl/health" -UseBasicParsing -TimeoutSec 2
      if ($response.StatusCode -eq 200) {
        $restartHealthy = $true
        break
      }
    } catch {
      Start-Sleep -Milliseconds 250
    }
  }
  $restartWatch.Stop()
  if (-not $restartHealthy) {
    throw "Backend did not recover within 15 seconds."
  }

  $oldTokenStatus = 0
  try {
    Invoke-WebRequest `
      -Uri "$BaseUrl/api/me" `
      -Headers @{ Authorization = "Bearer $oldToken" } `
      -UseBasicParsing `
      -TimeoutSec 5 | Out-Null
    $oldTokenStatus = 200
  } catch {
    if ($_.Exception.Response) {
      $oldTokenStatus = [int]$_.Exception.Response.StatusCode
    }
  }

  $loginAfterRestart = Invoke-RestMethod `
    -Uri "$BaseUrl/api/login" `
    -Method Post `
    -ContentType "application/json" `
    -Body $loginBody
  $newToken = [string]$loginAfterRestart.token
  $persistedReservations = Invoke-RestMethod `
    -Uri "$BaseUrl/api/admin/reservations?pageNo=1&pageSize=100" `
    -Headers @{ Authorization = "Bearer $newToken" } `
    -Method Get

  $restartSeconds = [math]::Round($restartWatch.Elapsed.TotalSeconds, 3)
  $recoveryPassed = (
    $restartSeconds -le 300 -and
    $oldTokenStatus -eq 401 -and
    [int]$persistedReservations.total -gt 6
  )
  $recoveryExit = if ($recoveryPassed) { 0 } else { 1 }
  [pscustomobject]@{
    measuredAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
    restartSeconds = $restartSeconds
    thresholdSeconds = 300
    oldTokenStatus = $oldTokenStatus
    persistedReservationTotal = [int]$persistedReservations.total
    passed = $recoveryPassed
  } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $resultsDir "recovery-result.json") -Encoding UTF8
} finally {
  $env:PATH = $oldPath
  if ($backendProcess -and -not $backendProcess.HasExited) {
    Stop-Process -Id $backendProcess.Id -Force
    $backendProcess.WaitForExit()
  }
}

$overallPassed = (
  $functionalExit -eq 0 -and
  $performanceExit -eq 0 -and
  $concurrencyExit -eq 0 -and
  $frontendExit -eq 0 -and
  $recoveryExit -eq 0
)

$summary = [pscustomobject]@{
  generatedAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
  testDatabase = $environment.TestDatabase
  startupSeconds = $startupSeconds
  functionalExitCode = $functionalExit
  performanceExitCode = $performanceExit
  concurrencyExitCode = $concurrencyExit
  frontendExitCode = $frontendExit
  recoveryExitCode = $recoveryExit
  passed = $overallPassed
}
$summary | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $resultsDir "t6-summary.json") -Encoding UTF8

Write-Host "T6 summary: startup=${startupSeconds}s, functional=$functionalExit, performance=$performanceExit, concurrency=$concurrencyExit, frontend=$frontendExit, recovery=$recoveryExit"
if (-not $overallPassed) {
  exit 1
}
