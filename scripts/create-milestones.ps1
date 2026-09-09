<#
.SYNOPSIS
    Create GitHub milestones from a list of strings.

.DESCRIPTION
    For each provided title, creates a milestone with that title in the target repository.
    Uses GitHub CLI (gh) and requires an authenticated session with permissions to the repo.

.PARAMETER Repo
    Target repository in the form "owner/repo" (e.g., "nam20485/agent-instructions").

.PARAMETER Titles
    An array of milestone titles to create.

.PARAMETER TitlesFile
    Path to a text file containing one milestone title per line. Empty lines and lines starting with '#' are ignored.

.PARAMETER Description
    Optional description applied to each created milestone.

.PARAMETER State
    Milestone state to set on creation. Allowed: open, closed. Default: open.

.PARAMETER DueOn
    Optional due date/time for each milestone. If provided, it's converted to UTC ISO 8601 (e.g., 2025-08-01T00:00:00Z).

.PARAMETER DryRun
    Show planned actions without making changes.

.PARAMETER SkipExisting
    When set, milestones that already exist (title match) will be skipped silently.

.EXAMPLE
    # Create milestones from a list
    ./scripts/create-milestones.ps1 -Repo "owner/repo" -Titles "Phase 1","Phase 2"

.EXAMPLE
    # Create milestones from a file and preview actions
    ./scripts/create-milestones.ps1 -Repo "owner/repo" -TitlesFile "./milestones.txt" -DryRun

.NOTES
    Requires: GitHub CLI (gh) and authenticated session (gh auth status).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[^/]+/[^/]+$')]
    [string]$Repo,

    [Parameter()]
    [string[]]$Titles,

    [Parameter()]
    [string]$TitlesFile,

    [Parameter()]
    [ValidateSet('open','closed')]
    [string]$State = 'open',

    [Parameter()]
    [string]$Description,

    [Parameter()]
    [DateTime]$DueOn,

    [switch]$DryRun,
    [switch]$SkipExisting
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

# Build the titles list from parameters
$allTitles = @()
if ($Titles) { $allTitles += $Titles }
if ($TitlesFile) {
    if (-not (Test-Path -LiteralPath $TitlesFile)) {
        Write-Error "Titles file not found: ${TitlesFile}"
        exit 1
    }
    $fileTitles = Get-Content -LiteralPath $TitlesFile | Where-Object { $_ -and (-not $_.TrimStart().StartsWith('#')) }
    $allTitles += $fileTitles
}

# Normalize, remove duplicates, trim
$allTitles = $allTitles | ForEach-Object { $_.Trim() } | Where-Object { $_ } | Select-Object -Unique

if (-not $allTitles -or $allTitles.Count -eq 0) {
    Write-Error "No milestone titles were provided. Use -Titles and/or -TitlesFile."
    exit 1
}

# Fetch existing milestones (all states, up to 100 per page; --paginate for more)
try {
    $existing = gh api "repos/$Repo/milestones?state=all&per_page=100" --paginate | ConvertFrom-Json
} catch {
    Write-Error "Failed to fetch existing milestones from repo '$Repo'. Ensure you have access and are authenticated."
    exit 1
}

$existingByTitle = @{}
foreach ($m in $existing) {
    if ($m.title) { $existingByTitle[$m.title.ToLowerInvariant()] = $true }
}

# Format due date if provided
$dueIso = $null
if ($PSBoundParameters.ContainsKey('DueOn')) {
    # Convert to UTC ISO 8601 with 'Z'
    $dueIso = ($DueOn.ToUniversalTime()).ToString("yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z'")
}

$planned = @()
foreach ($t in $allTitles) {
    $key = $t.ToLowerInvariant()
    if ($existingByTitle.ContainsKey($key)) {
        if (-not $SkipExisting) {
            $planned += @{ action='skip'; title=$t; reason='already exists' }
        }
        continue
    }
    $planned += @{ action='create'; title=$t }
}

if ($planned.Count -eq 0) {
    Write-Host "No milestones to create." -ForegroundColor Green
    exit 0
}

Write-Host "Planned actions:" -ForegroundColor Cyan
$planned | ForEach-Object { Write-Host (" - {0}: {1}{2}" -f $_.action, $_.title, $(if ($_.reason) { " ($($_.reason))" } else { '' })) }

if ($DryRun) {
    Write-Host "Dry run specified. No changes made." -ForegroundColor Yellow
    exit 0
}

foreach ($p in $planned) {
    if ($p.action -ne 'create') { continue }
    $title = $p.title
    Write-Host "Creating milestone '$title'..." -ForegroundColor Cyan

    $ghArgs = @(
        'api', "repos/$Repo/milestones", '-X', 'POST',
        '-f', "title=$title",
        '-f', "state=$State"
    )
    if ($Description) { $ghArgs += @('-f', "description=$Description") }
    if ($dueIso) { $ghArgs += @('-f', "due_on=$dueIso") }

    & gh @ghArgs | Out-Null
}

Write-Host "Done." -ForegroundColor Green
