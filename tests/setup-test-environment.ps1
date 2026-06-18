param(
  [string]$ProjectRoot = "",
  [string]$TestDatabase = "sports_equipment_management_test"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
  $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Read-IniLikeConfig {
  param([string]$Path)

  $result = @{}
  foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
    $trimmed = $line.Trim()
    if ($trimmed.Length -eq 0 -or $trimmed.StartsWith("#")) {
      continue
    }

    $separator = $trimmed.IndexOf("=")
    if ($separator -lt 1) {
      continue
    }

    $key = $trimmed.Substring(0, $separator).Trim()
    $value = $trimmed.Substring($separator + 1).Trim()
    $result[$key] = $value
  }
  return $result
}

$baseConfigPath = Join-Path $ProjectRoot "backend\config\database.conf"
$schemaPath = Join-Path $ProjectRoot "database\schema\01_init.sql"
$seedPath = Join-Path $ProjectRoot "database\seed\01_seed.sql"
$runtimeRoot = Join-Path $PSScriptRoot "runtime"
$runtimeBackend = Join-Path $runtimeRoot "backend"
$runtimeConfigDir = Join-Path $runtimeBackend "config"

New-Item -ItemType Directory -Force -Path $runtimeConfigDir | Out-Null

$config = Read-IniLikeConfig -Path $baseConfigPath
$mysqlCommand = Get-Command mysql -ErrorAction SilentlyContinue
if (-not $mysqlCommand) {
  throw "mysql client was not found in PATH."
}

$mysqlArgs = @(
  "--host=$($config.host)",
  "--port=$($config.port)",
  "--user=$($config.username)",
  "--default-character-set=utf8mb4"
)

$previousMysqlPassword = $env:MYSQL_PWD
$env:MYSQL_PWD = $config.password

try {
  & $mysqlCommand.Source @mysqlArgs -e "DROP DATABASE IF EXISTS ``$TestDatabase``;"
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to reset test database."
  }

  $schemaText = Get-Content -LiteralPath $schemaPath -Raw -Encoding UTF8
  $seedText = Get-Content -LiteralPath $seedPath -Raw -Encoding UTF8
  $schemaText = $schemaText.Replace("sports_equipment_management", $TestDatabase)
  $seedText = $seedText.Replace("sports_equipment_management", $TestDatabase)

  $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
  $runtimeSchema = Join-Path $runtimeRoot "01_init_test.sql"
  $runtimeSeed = Join-Path $runtimeRoot "01_seed_test.sql"
  [System.IO.File]::WriteAllText($runtimeSchema, $schemaText, $utf8NoBom)
  [System.IO.File]::WriteAllText($runtimeSeed, $seedText, $utf8NoBom)

  $schemaForMysql = $runtimeSchema.Replace("\", "/")
  $seedForMysql = $runtimeSeed.Replace("\", "/")

  & $mysqlCommand.Source @mysqlArgs -e "source $schemaForMysql"
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to import test schema."
  }

  & $mysqlCommand.Source @mysqlArgs -e "source $seedForMysql"
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to import test seed data."
  }

  $runtimeConfig = @(
    "host=$($config.host)"
    "port=$($config.port)"
    "database=$TestDatabase"
    "username=$($config.username)"
    "password=$($config.password)"
    "charset=$($config.charset)"
  ) -join [Environment]::NewLine

  $runtimeConfigPath = Join-Path $runtimeConfigDir "database.conf"
  [System.IO.File]::WriteAllText(
    $runtimeConfigPath,
    $runtimeConfig + [Environment]::NewLine,
    $utf8NoBom
  )

  [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    RuntimeBackend = $runtimeBackend
    RuntimeConfig = $runtimeConfigPath
    TestDatabase = $TestDatabase
    Mysql = $mysqlCommand.Source
  }
} finally {
  $env:MYSQL_PWD = $previousMysqlPassword
}
