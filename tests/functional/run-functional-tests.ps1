param(
  [string]$BaseUrl = "http://127.0.0.1:8000",
  [string]$OutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Net.Http

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
  $OutputPath = Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "results\functional-results.json"
}

$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$client = New-Object System.Net.Http.HttpClient
$client.Timeout = [TimeSpan]::FromSeconds(10)
$results = New-Object System.Collections.Generic.List[object]

$studentToken = ""
$student2Token = ""
$adminToken = ""
$initialStock = 0
$flowReservationId = 0
$flowBorrowId = 0
$cancelReservationId = 0
$pendingReservationId = 0

function Send-Api {
  param(
    [string]$Method,
    [string]$Path,
    [object]$Body = $null,
    [string]$Token = ""
  )

  $httpMethod = New-Object System.Net.Http.HttpMethod -ArgumentList $Method
  $request = New-Object System.Net.Http.HttpRequestMessage -ArgumentList @(
    $httpMethod,
    ($BaseUrl + $Path)
  )

  if (-not [string]::IsNullOrWhiteSpace($Token)) {
    $null = $request.Headers.TryAddWithoutValidation("Authorization", "Bearer $Token")
  }

  if ($null -ne $Body) {
    $json = $Body | ConvertTo-Json -Depth 10 -Compress
    $request.Content = New-Object System.Net.Http.StringContent -ArgumentList @(
      $json,
      [System.Text.Encoding]::UTF8,
      "application/json"
    )
  }

  $response = $client.SendAsync($request).GetAwaiter().GetResult()
  $text = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
  $parsed = $null
  if (-not [string]::IsNullOrWhiteSpace($text)) {
    try {
      $parsed = $text | ConvertFrom-Json
    } catch {
      $parsed = $null
    }
  }

  [pscustomobject]@{
    Status = [int]$response.StatusCode
    Text = $text
    Json = $parsed
  }
}

function Add-Case {
  param(
    [string]$Id,
    [string]$Description,
    [scriptblock]$Action
  )

  $startedAt = Get-Date
  try {
    $detail = & $Action
    $passed = $true
  } catch {
    $passed = $false
    $detail = $_.Exception.Message
  }

  $durationMs = [math]::Round(((Get-Date) - $startedAt).TotalMilliseconds, 2)
  $result = [pscustomobject]@{
    id = $Id
    description = $Description
    passed = $passed
    durationMs = $durationMs
    detail = [string]$detail
  }
  $results.Add($result)

  $label = if ($passed) { "PASS" } else { "FAIL" }
  Write-Host "[$label] $Id $Description - $detail"
}

function Expect-Status {
  param(
    [object]$Response,
    [int]$Expected
  )

  if ($Response.Status -ne $Expected) {
    throw "expected HTTP $Expected, actual $($Response.Status), body=$($Response.Text)"
  }
}

try {
  Add-Case "F-01" "Health endpoint returns 200" {
    $response = Send-Api "GET" "/health"
    Expect-Status $response 200
    "HTTP 200"
  }

  Add-Case "F-02" "Invalid password is rejected" {
    $response = Send-Api "POST" "/api/login" @{
      username = "stu2026001"
      password = "wrong-password"
    }
    Expect-Status $response 401
    "HTTP 401"
  }

  Add-Case "F-03" "Student login succeeds" {
    $response = Send-Api "POST" "/api/login" @{
      username = "stu2026001"
      password = "Student@123"
    }
    Expect-Status $response 200
    if (-not $response.Json.token) {
      throw "login response has no token"
    }
    $script:studentToken = [string]$response.Json.token
    "role=$($response.Json.user.role)"
  }

  Add-Case "F-04" "Second student login succeeds" {
    $response = Send-Api "POST" "/api/login" @{
      username = "stu2026002"
      password = "Student@123"
    }
    Expect-Status $response 200
    $script:student2Token = [string]$response.Json.token
    "role=$($response.Json.user.role)"
  }

  Add-Case "F-05" "Admin login succeeds" {
    $response = Send-Api "POST" "/api/login" @{
      username = "admin01"
      password = "Admin@123"
    }
    Expect-Status $response 200
    $script:adminToken = [string]$response.Json.token
    "role=$($response.Json.user.role)"
  }

  Add-Case "F-06" "Token resolves current user" {
    $response = Send-Api "GET" "/api/me" $null $studentToken
    Expect-Status $response 200
    if ($response.Json.username -ne "stu2026001") {
      throw "unexpected current user"
    }
    "username=$($response.Json.username)"
  }

  Add-Case "F-07" "Equipment categories are public" {
    $response = Send-Api "GET" "/api/equipment-categories"
    Expect-Status $response 200
    if ($response.Json.items.Count -lt 1) {
      throw "category list is empty"
    }
    "categories=$($response.Json.items.Count)"
  }

  Add-Case "F-08" "Equipment page query is public" {
    $response = Send-Api "GET" "/api/equipment?pageNo=1&pageSize=10"
    Expect-Status $response 200
    $equipment = $response.Json.items | Where-Object { $_.id -eq 1001 } | Select-Object -First 1
    if (-not $equipment) {
      throw "equipment 1001 was not found"
    }
    $script:initialStock = [int]$equipment.availableStock
    "total=$($response.Json.total), equipment1001Stock=$initialStock"
  }

  Add-Case "F-09" "Invalid paging returns 400" {
    $response = Send-Api "GET" "/api/equipment?pageNo=0&pageSize=10"
    Expect-Status $response 400
    "HTTP 400"
  }

  Add-Case "F-10" "Missing authorization header is rejected" {
    $response = Send-Api "GET" "/api/reservations/my"
    Expect-Status $response 400
    "HTTP 400 from required Oat++ header binding"
  }

  Add-Case "F-11" "Student cannot access admin reservations" {
    $response = Send-Api "GET" "/api/admin/reservations" $null $studentToken
    Expect-Status $response 403
    "HTTP 403"
  }

  Add-Case "F-12" "Admin cannot create a student reservation" {
    $response = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1001
      reservationStartAt = "2026-12-01 10:00:00"
      reservationEndAt = "2026-12-01 12:00:00"
      quantity = 1
      requestNote = "role test"
    } $adminToken
    Expect-Status $response 403
    "HTTP 403"
  }

  Add-Case "F-13" "Invalid reservation time returns 400" {
    $response = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1001
      reservationStartAt = "bad-time"
      reservationEndAt = "2026-12-01 12:00:00"
      quantity = 1
      requestNote = "invalid time"
    } $studentToken
    Expect-Status $response 400
    "HTTP 400"
  }

  $baseDate = (Get-Date).Date.AddDays(30).AddHours(10)
  $flowStart = $baseDate.ToString("yyyy-MM-dd HH:mm:ss")
  $flowEnd = $baseDate.AddHours(2).ToString("yyyy-MM-dd HH:mm:ss")

  Add-Case "F-14" "Reservation over stock returns 409" {
    $response = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1001
      reservationStartAt = $flowStart
      reservationEndAt = $flowEnd
      quantity = 999
      requestNote = "over stock"
    } $studentToken
    Expect-Status $response 409
    "HTTP 409"
  }

  Add-Case "F-15" "Student creates reservation" {
    $response = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1001
      reservationStartAt = $flowStart
      reservationEndAt = $flowEnd
      quantity = 1
      requestNote = "T6 full flow"
    } $studentToken
    Expect-Status $response 201
    $script:flowReservationId = [uint64]$response.Json.id
    if ($response.Json.status -ne "PENDING") {
      throw "new reservation is not PENDING"
    }
    "reservationId=$flowReservationId"
  }

  Add-Case "F-16" "Another student cannot view the reservation" {
    $response = Send-Api "GET" "/api/reservations/my/$flowReservationId" $null $student2Token
    Expect-Status $response 404
    "HTTP 404"
  }

  Add-Case "F-17" "Admin approves reservation" {
    $response = Send-Api "POST" "/api/admin/reservations/$flowReservationId/approve" @{
      reviewNote = "T6 approved"
    } $adminToken
    Expect-Status $response 200
    if ($response.Json.status -ne "APPROVED") {
      throw "reservation is not APPROVED"
    }
    "status=$($response.Json.status)"
  }

  Add-Case "F-18" "Repeated approval is rejected" {
    $response = Send-Api "POST" "/api/admin/reservations/$flowReservationId/approve" @{
      reviewNote = "approve again"
    } $adminToken
    Expect-Status $response 409
    "HTTP 409"
  }

  $borrowedAt = (Get-Date).AddMinutes(1).ToString("yyyy-MM-dd HH:mm:ss")
  $dueAt = (Get-Date).AddHours(3).ToString("yyyy-MM-dd HH:mm:ss")

  Add-Case "F-19" "Admin creates borrow record" {
    $response = Send-Api "POST" "/api/admin/borrows" @{
      reservationId = $flowReservationId
      borrowedAt = $borrowedAt
      dueAt = $dueAt
    } $adminToken
    Expect-Status $response 201
    $script:flowBorrowId = [uint64]$response.Json.id
    if ($response.Json.status -ne "BORROWING") {
      throw "borrow record is not BORROWING"
    }
    "borrowId=$flowBorrowId"
  }

  Add-Case "F-20" "Borrow decreases stock exactly once" {
    $response = Send-Api "GET" "/api/equipment/1001"
    Expect-Status $response 200
    $actualStock = [int]$response.Json.availableStock
    if ($actualStock -ne ($initialStock - 1)) {
      throw "expected stock $($initialStock - 1), actual $actualStock"
    }
    "stock=$actualStock"
  }

  Add-Case "F-21" "Reservation cannot be borrowed twice" {
    $response = Send-Api "POST" "/api/admin/borrows" @{
      reservationId = $flowReservationId
      borrowedAt = $borrowedAt
      dueAt = $dueAt
    } $adminToken
    Expect-Status $response 409
    "HTTP 409"
  }

  Add-Case "F-22" "Student can query own borrow record" {
    $response = Send-Api "GET" '/api/borrows/my?pageNo=1&pageSize=100' $null $studentToken
    Expect-Status $response 200
    $record = $response.Json.items | Where-Object { $_.id -eq $flowBorrowId } | Select-Object -First 1
    if (-not $record) {
      throw "created borrow record was not found"
    }
    "borrowId=$flowBorrowId"
  }

  $returnedAt = (Get-Date).AddMinutes(2).ToString("yyyy-MM-dd HH:mm:ss")
  Add-Case "F-23" "Admin returns borrow record" {
    $response = Send-Api "POST" "/api/admin/borrows/$flowBorrowId/return" @{
      returnedAt = $returnedAt
      returnNote = "T6 returned"
    } $adminToken
    Expect-Status $response 200
    if ($response.Json.status -ne "RETURNED") {
      throw "borrow record is not RETURNED"
    }
    "status=$($response.Json.status)"
  }

  Add-Case "F-24" "Return restores stock" {
    $response = Send-Api "GET" "/api/equipment/1001"
    Expect-Status $response 200
    $actualStock = [int]$response.Json.availableStock
    if ($actualStock -ne $initialStock) {
      throw "expected stock $initialStock, actual $actualStock"
    }
    "stock=$actualStock"
  }

  Add-Case "F-25" "Borrow record cannot be returned twice" {
    $response = Send-Api "POST" "/api/admin/borrows/$flowBorrowId/return" @{
      returnedAt = $returnedAt
      returnNote = "return again"
    } $adminToken
    Expect-Status $response 409
    "HTTP 409"
  }

  $cancelDate = $baseDate.AddDays(1)
  Add-Case "F-26" "Student cancels pending reservation" {
    $created = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1003
      reservationStartAt = $cancelDate.ToString("yyyy-MM-dd HH:mm:ss")
      reservationEndAt = $cancelDate.AddHours(1).ToString("yyyy-MM-dd HH:mm:ss")
      quantity = 1
      requestNote = "T6 cancel flow"
    } $studentToken
    Expect-Status $created 201
    $script:cancelReservationId = [uint64]$created.Json.id

    $canceled = Send-Api "POST" "/api/reservations/my/$cancelReservationId/cancel" @{
      cancelReason = "T6 cancel"
    } $studentToken
    Expect-Status $canceled 200
    if ($canceled.Json.status -ne "CANCELED") {
      throw "reservation is not CANCELED"
    }
    "reservationId=$cancelReservationId"
  }

  Add-Case "F-27" "Canceled reservation cannot be canceled twice" {
    $response = Send-Api "POST" "/api/reservations/my/$cancelReservationId/cancel" @{
      cancelReason = "cancel again"
    } $studentToken
    Expect-Status $response 409
    "HTTP 409"
  }

  $pendingDate = $baseDate.AddDays(2)
  Add-Case "F-28" "Pending reservation cannot be borrowed" {
    $created = Send-Api "POST" "/api/reservations" @{
      equipmentId = 1006
      reservationStartAt = $pendingDate.ToString("yyyy-MM-dd HH:mm:ss")
      reservationEndAt = $pendingDate.AddHours(1).ToString("yyyy-MM-dd HH:mm:ss")
      quantity = 1
      requestNote = "T6 pending borrow rejection"
    } $studentToken
    Expect-Status $created 201
    $script:pendingReservationId = [uint64]$created.Json.id

    $borrow = Send-Api "POST" "/api/admin/borrows" @{
      reservationId = $pendingReservationId
      borrowedAt = $borrowedAt
      dueAt = $dueAt
    } $adminToken
    Expect-Status $borrow 409
    "HTTP 409"
  }

  Add-Case "F-29" "Logout invalidates the old token" {
    $logout = Send-Api "POST" "/api/logout" $null $studentToken
    Expect-Status $logout 200
    $me = Send-Api "GET" "/api/me" $null $studentToken
    Expect-Status $me 401
    "logout=200, oldToken=401"
  }
} finally {
  $client.Dispose()
}

$passedCount = @($results | Where-Object { $_.passed }).Count
$failedCount = $results.Count - $passedCount
$summary = [pscustomobject]@{
  generatedAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
  baseUrl = $BaseUrl
  total = $results.Count
  passed = $passedCount
  failed = $failedCount
  cases = $results
}

$summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
Write-Host "Functional summary: total=$($summary.total), passed=$passedCount, failed=$failedCount"

if ($failedCount -gt 0) {
  exit 1
}
