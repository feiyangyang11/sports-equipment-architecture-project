param(
  [string]$FrontendDir,
  [string]$OutputPath,
  [int]$Port = 5500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$python = Get-Command python -ErrorAction Stop
$baseUrl = "http://127.0.0.1:$Port"
$resultDirectory = Split-Path -Parent $OutputPath
$runtimeDirectory = Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "runtime"
New-Item -ItemType Directory -Force -Path $resultDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $runtimeDirectory | Out-Null

$serverOut = Join-Path $resultDirectory "frontend.stdout.log"
$serverErr = Join-Path $resultDirectory "frontend.stderr.log"
$server = $null

$browsers = @(
  [pscustomobject]@{
    name = "Edge"
    path = "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
  },
  [pscustomobject]@{
    name = "Chrome"
    path = "C:\Program Files\Google\Chrome\Application\chrome.exe"
  }
)

$staticPaths = @(
  "/pages/login.html",
  "/pages/student-equipment.html",
  "/pages/student-reservations.html",
  "/pages/student-borrows.html",
  "/pages/admin-reservations.html",
  "/pages/admin-borrows.html",
  "/assets/css/app.css",
  "/assets/js/api.js"
)

try {
  $server = Start-Process `
    -FilePath $python.Source `
    -ArgumentList @("-m", "http.server", "$Port", "-d", $FrontendDir) `
    -WorkingDirectory $FrontendDir `
    -WindowStyle Hidden `
    -RedirectStandardOutput $serverOut `
    -RedirectStandardError $serverErr `
    -PassThru

  $ready = $false
  for ($attempt = 0; $attempt -lt 40; $attempt++) {
    try {
      $response = Invoke-WebRequest -Uri "$baseUrl/pages/login.html" -UseBasicParsing -TimeoutSec 2
      if ($response.StatusCode -eq 200) {
        $ready = $true
        break
      }
    } catch {
      Start-Sleep -Milliseconds 250
    }
  }
  if (-not $ready) {
    throw "Frontend static server did not become ready."
  }

  $staticResults = foreach ($path in $staticPaths) {
    try {
      $response = Invoke-WebRequest -Uri "$baseUrl$path" -UseBasicParsing -TimeoutSec 5
      [pscustomobject]@{
        path = $path
        status = [int]$response.StatusCode
        passed = $response.StatusCode -eq 200
      }
    } catch {
      [pscustomobject]@{
        path = $path
        status = 0
        passed = $false
      }
    }
  }

  $browserResults = foreach ($browser in $browsers) {
    if (-not (Test-Path -LiteralPath $browser.path)) {
      [pscustomobject]@{
        browser = $browser.name
        available = $false
        version = ""
        exitCode = -1
        domContainsLoginForm = $false
        passed = $false
      }
      continue
    }

    $profileDirectory = Join-Path $runtimeDirectory ("browser-" + $browser.name.ToLowerInvariant())
    $domPath = Join-Path $resultDirectory ($browser.name.ToLowerInvariant() + "-login-dom.html")
    $errorPath = Join-Path $resultDirectory ($browser.name.ToLowerInvariant() + "-headless.stderr.log")
    $arguments = @(
      "--headless=new",
      "--disable-gpu",
      "--no-first-run",
      "--user-data-dir=$profileDirectory",
      "--dump-dom",
      "$baseUrl/pages/login.html"
    )

    $process = Start-Process `
      -FilePath $browser.path `
      -ArgumentList $arguments `
      -WindowStyle Hidden `
      -RedirectStandardOutput $domPath `
      -RedirectStandardError $errorPath `
      -Wait `
      -PassThru

    $dom = if (Test-Path -LiteralPath $domPath) {
      Get-Content -LiteralPath $domPath -Raw -Encoding UTF8
    } else {
      ""
    }
    $containsLoginForm = $dom.Contains('id="loginForm"')

    [pscustomobject]@{
      browser = $browser.name
      available = $true
      version = (Get-Item -LiteralPath $browser.path).VersionInfo.ProductVersion
      exitCode = $process.ExitCode
      domContainsLoginForm = $containsLoginForm
      passed = $process.ExitCode -eq 0 -and $containsLoginForm
    }
  }

  $passed = (
    @($staticResults | Where-Object { -not $_.passed }).Count -eq 0 -and
    @($browserResults | Where-Object { -not $_.passed }).Count -eq 0
  )

  $result = [pscustomobject]@{
    generatedAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
    baseUrl = $baseUrl
    staticResources = $staticResults
    browsers = $browserResults
    passed = $passed
  }
  $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
  $result | ConvertTo-Json -Depth 10

  if (-not $passed) {
    exit 1
  }
} finally {
  if ($server -and -not $server.HasExited) {
    Stop-Process -Id $server.Id -Force
    $server.WaitForExit()
  }
}
