<#
.SYNOPSIS
    Search winget packages (source: winget only).

.DESCRIPTION
    Outputs results in format: ID|Name|Version

.PARAMETER Query
    Search query string.

.PARAMETER Add
    If set, adds the FIRST result to repositories.txt.

.PARAMETER AddAll
    If set, adds ALL results to repositories.txt.

.PARAMETER AddId
    If set, adds the result matching this ID to repositories.txt.

.EXAMPLE
    .\search_winget.ps1 -Query "vmware"

.EXAMPLE
    .\search_winget.ps1 -Query "vlc" -Add

.EXAMPLE
    .\search_winget.ps1 -Query "media player" -AddAll

.EXAMPLE
    .\search_winget.ps1 -Query "vlc" -AddId "VideoLAN.VLC"
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Query,

    [switch]$Add,
    [switch]$AddAll,

    [string]$AddId = ""
)

$ErrorActionPreference = "SilentlyContinue"

# Путь к repositories.txt (рядом со скриптом)
$RepoFile = Join-Path $PSScriptRoot "repositories.txt"

# Проверка winget
$winget = Get-Command winget -ErrorAction SilentlyContinue
if (-not $winget) {
    Write-Error "winget not found"
    exit 1
}

# ============================================================
# Поиск в winget
# ============================================================
$output = winget search --query $Query --source winget `
                        --accept-source-agreements `
                        --disable-interactivity 2>$null

if (-not $output) {
    exit 0
}

$results = @()
$started = $false

foreach ($line in $output) {
    # Ждём разделитель "---"
    if ($line -match '^-{3,}') {
        $started = $true
        continue
    }
    if (-not $started) { continue }
    if ([string]::IsNullOrWhiteSpace($line)) { continue }

    # Разбиваем по 2+ пробелам
    $parts = $line -split '\s{2,}' | Where-Object { $_ -ne '' }
    if ($parts.Count -lt 2) { continue }

    $name = $parts[0].Trim()
    $id   = $parts[1].Trim()
    $ver  = if ($parts.Count -ge 3) { $parts[2].Trim() } else { "" }

    # Пропускаем строки, где ID не похож на winget ID (нет точки)
    if ($id -notmatch '\.') { continue }

    $results += [PSCustomObject]@{
        ID      = $id
        Name    = $name
        Version = $ver
    }
}

# ============================================================
# Режим: только вывод
# ============================================================
if (-not $Add -and -not $AddAll -and -not $AddId) {
    foreach ($r in $results) {
        Write-Output "$($r.ID)|$($r.Name)|$($r.Version)"
    }
    exit 0
}

# ============================================================
# Проверка файла репозитория
# ============================================================
if (-not (Test-Path $RepoFile)) {
    Write-Error "repositories.txt not found: $RepoFile"
    exit 1
}

# ============================================================
# Добавление: --add-all
# ============================================================
if ($AddAll) {
    Add-Content -Path $RepoFile -Value ""
    Add-Content -Path $RepoFile -Value "# --- Added by search on $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ---"
    foreach ($r in $results) {
        Add-Content -Path $RepoFile -Value "$($r.ID)|$($r.Name)"
        Write-Output "Added: $($r.ID) ($($r.Name))"
    }
    Write-Output "[OK] All results added to: $RepoFile"
    exit 0
}

# ============================================================
# Добавление одной записи: --add или --add-id
# ============================================================
$targetId = ""
if ($AddId -ne "") {
    $targetId = $AddId
} elseif ($Add) {
    if ($results.Count -eq 0) {
        Write-Error "No results to add"
        exit 1
    }
    $targetId = $results[0].ID
}

if ($targetId -eq "") {
    Write-Error "No target ID to add"
    exit 1
}

# Ищем запись в результатах
$target = $results | Where-Object { $_.ID -eq $targetId } | Select-Object -First 1

if (-not $target) {
    # Если не нашли — берём ID как есть, без имени
    $targetName = $targetId
} else {
    $targetName = $target.Name
}

# Проверяем, нет ли уже в файле
$existing = Get-Content $RepoFile -ErrorAction SilentlyContinue | Where-Object {
    $_ -match "^\s*$([regex]::Escape($targetId))\s*\|"
}

if ($existing) {
    Write-Output "[SKIP] $targetId already in repositories.txt"
    exit 0
}

Add-Content -Path $RepoFile -Value ""
Add-Content -Path $RepoFile -Value "$targetId|$targetName"

Write-Output "[OK] Added: $targetId ($targetName)"
exit 0