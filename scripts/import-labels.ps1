<#
.SYNOPSIS
    Import labels into a GitHub repository from a JSON export file.

.DESCRIPTION
    Reads a JSON file (array of label objects as exported via:
        gh api repos/<owner>/<repo>/labels --paginate > .labels.json
    ) and ensures the target repository has the same labels.

    - Creates missing labels
    - Updates color/description when they differ
    - Optionally deletes labels not present in the source file

.PARAMETER Repo
    Target repository in the form "owner/repo" (e.g., "nam20485/agent-instructions").

.PARAMETER LabelsFile
    Path to the JSON labels file (default: ./.labels.json).

.PARAMETER DryRun
    Show what would change without making any changes.

.PARAMETER DeleteMissing
    Delete labels in the target repo that are not present in the source file.

.EXAMPLE
    # Preview changes
    ./scripts/import-labels.ps1 -Repo "owner/target-repo" -LabelsFile "./.labels.json" -DryRun

.EXAMPLE
    # Apply changes (create/update)
    ./scripts/import-labels.ps1 -Repo "owner/target-repo" -LabelsFile "./.labels.json"

.EXAMPLE
    # Apply changes and delete labels not in the file
    ./scripts/import-labels.ps1 -Repo "owner/target-repo" -DeleteMissing

.NOTES
    Requires GitHub CLI (gh) and an authenticated session with access to the target repo:
        gh auth status
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[^/]+/[^/]+$')]
    [string]$Repo,

    [Parameter()]
    [string]$LabelsFile = "./.labels.json",

    [switch]$DryRun,
    [switch]$DeleteMissing
)

function Test-CommandExists {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' not found in PATH. Please install it first."
    }
}

try {
    Test-CommandExists gh
} catch {
    Write-Error $_.Exception.Message
    exit 1
}

# Dot-source common auth helper if present
$commonAuth = Join-Path $PSScriptRoot 'common-auth.ps1'
if (Test-Path -LiteralPath $commonAuth) { . $commonAuth }
if (Get-Command Initialize-GitHubAuth -ErrorAction SilentlyContinue) { Initialize-GitHubAuth } else {
    $st = & gh auth status 2>$null; $code = $LASTEXITCODE
    if ($code -ne 0) { Write-Warning 'GitHub CLI not authenticated. Initiating gh auth login...'; & gh auth login }
}

if (-not (Test-Path -LiteralPath $LabelsFile)) {
    Write-Error "Labels file not found: $LabelsFile"
    exit 1
}

# Load labels from file
try {
    $sourceLabels = Get-Content -LiteralPath ${LabelsFile} -Raw | ConvertFrom-Json
} catch {
    Write-Error "Failed to parse JSON from ${LabelsFile}: $($_.Exception.Message)"
    exit 1
}

if (-not $sourceLabels) {
    Write-Warning "No labels found in file: $LabelsFile"
    exit 0
}

# Load existing labels from target repo (up to 100)
try {
    $existingLabels = gh api "repos/$Repo/labels" --paginate | ConvertFrom-Json
} catch {
    Write-Error "Failed to fetch labels from repo '$Repo'. Ensure you have access and are authenticated."
    exit 1
}

# Build lookup by name (case-insensitive)
$existingByName = @{}
foreach ($l in $existingLabels) {
    $existingByName[$l.name.ToLowerInvariant()] = $l
}

# Normalize helper
function ConvertTo-NormalizedColor {
    param([string]$Color)
    if (-not $Color) { return $null }
    $c = $Color.Trim()
    if ($c.StartsWith('#')) { $c = $c.Substring(1) }
    return $c.ToLowerInvariant()
}

$actions = @()

foreach ($label in $sourceLabels) {
    $name = [string]$label.name
    if (-not $name) { continue }

    $color = ConvertTo-NormalizedColor -Color $label.color
    $desc  = if ($null -ne $label.description) { [string]$label.description } else { '' }

    $key = $name.ToLowerInvariant()
    $existing = $existingByName[$key]

    if (-not $existing) {
        $actions += @{ action='create'; name=$name; color=$color; description=$desc }
        continue
    }

    $needsUpdate = $false

    $existingColor = ConvertTo-NormalizedColor -Color $existing.color
    if ($existingColor -ne $color) { $needsUpdate = $true }

    # Normalize description comparisons: null vs empty
    $existingDesc = if ($null -ne $existing.description) { [string]$existing.description } else { '' }
    if ($existingDesc -ne $desc) { $needsUpdate = $true }

    if ($needsUpdate) {
        $actions += @{ action='update'; name=$name; color=$color; description=$desc }
    }
}

if ($DeleteMissing) {
    # Delete any target labels not present in source
    $sourceNames = @{}
    foreach ($s in $sourceLabels) {
        if ($s.name) { $sourceNames[$s.name.ToLowerInvariant()] = $true }
    }

    foreach ($e in $existingLabels) {
        $ekey = $e.name.ToLowerInvariant()
        if (-not $sourceNames.ContainsKey($ekey)) {
            $actions += @{ action='delete'; name=$e.name }
        }
    }
}

if ($actions.Count -eq 0) {
    Write-Host "No changes required. Target repo labels are up to date." -ForegroundColor Green
    exit 0
}

Write-Host "Planned actions:" -ForegroundColor Cyan
$actions | ForEach-Object { Write-Host (" - {0}: {1}" -f $_.action, $_.name) }

if ($DryRun) {
    Write-Host "Dry run specified. No changes made." -ForegroundColor Yellow
    exit 0
}

foreach ($a in $actions) {
    $name = $a.name
    $encoded = [uri]::EscapeDataString($name)

    switch ($a.action) {
        'create' {
            Write-Host "Creating label '$name'..." -ForegroundColor Cyan
            $ghArgs = @(
                'api', "repos/$Repo/labels", '-X', 'POST',
                '-f', "name=$name"
            )
            if ($a.color) { $ghArgs += @('-f', "color=$($a.color)") }
            if ($null -ne $a.description) { $ghArgs += @('-f', "description=$($a.description)") }
            & gh @ghArgs | Out-Null
        }
        'update' {
            Write-Host "Updating label '$name'..." -ForegroundColor Cyan
            $ghArgs = @(
                'api', "repos/$Repo/labels/$encoded", '-X', 'PATCH',
                '-f', "new_name=$name"
            )
            if ($a.color) { $ghArgs += @('-f', "color=$($a.color)") }
            if ($null -ne $a.description) { $ghArgs += @('-f', "description=$($a.description)") }
            & gh @ghArgs | Out-Null
        }
        'delete' {
            Write-Host "Deleting label '$name'..." -ForegroundColor Yellow
            & gh api "repos/$Repo/labels/$encoded" -X DELETE | Out-Null
        }
    }
}

Write-Host "Done." -ForegroundColor Green
