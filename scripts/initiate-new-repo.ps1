#requires -Version 7.0
<#
.SYNOPSIS
Orchestrates the entire "initiate-new-repository" dynamic workflow end-to-end.

.DESCRIPTION
This script performs all steps described by the initiate-new-repository dynamic workflow using
GitHub CLI (gh) and local helper scripts. It is idempotent where possible and provides a DryRun mode.

High-level steps:
- Validate environment and prerequisites
- Create owner-level GitHub Project (no template) named after the repo
- Create a new repository from a template (default: nam20485/ai-new-app-template), public with AGPL-3.0 license
- Ensure default branch is set (default: development)
- Clone the repository under <workspaceRoot>\dynamic_workflows\<RepoName>
- Copy seed docs (advanced_memory plan + index) into repo/docs/
- Import labels from .labels.json using scripts/import-labels.ps1
- Create milestones using scripts/create-milestones.ps1
- Commit and push seeded content to the default branch
- Run post-clone initialization script scripts/init-template-repo.ps1

.PARAMETER RepoName
Name of the new repository to create in the GitHub organization.

.PARAMETER Owner
GitHub organization or user that will own the repository. Default: nam20485

.PARAMETER Template
Template repository to use. Default: nam20485/ai-new-app-template

.PARAMETER Public
Create repository as public. Default: true (use -Private to override)

.PARAMETER Private
Create repository as private. Overrides -Public when set.

.PARAMETER License
SPDX license identifier for the repository license. Default: agpl-3.0

.PARAMETER DefaultBranch
Default branch to set on the repository. Default: development

.PARAMETER WorkspaceRoot
Optional explicit workspace root. If not provided, script will try git rev-parse --show-toplevel, else current directory.

.PARAMETER MilestoneTitles
Array of milestone titles to create. Default includes 3 standard phases.

.PARAMETER MilestoneDueDates
Array of due dates (YYYY-MM-DD) matching MilestoneTitles order. Must be same length.

.PARAMETER SkipProject
Skip creating the owner-level GitHub Project.

.PARAMETER SkipRepoCreate
Skip creating the repository (assume it exists).

.PARAMETER SkipClone
Skip cloning the repository (assume it's already cloned at the destination).

.PARAMETER SkipDocs
Skip copying seed docs.

.PARAMETER SkipLabels
Skip importing labels.

.PARAMETER SkipMilestones
Skip creating milestones.

.PARAMETER SkipInitTemplateScript
Skip running the post-clone scripts/init-template-repo.ps1 step.

.PARAMETER DryRun
When set, commands that would change remote state are not executed; actions are logged instead.

.EXAMPLE
./scripts/initiate-new-repo.ps1 -RepoName my-new-repo

.EXAMPLE
./scripts/initiate-new-repo.ps1 -RepoName my-new-repo -DryRun -Verbose

#>

[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [ValidateNotNullOrEmpty()]
  [string]$RepoName,

  [string]$Owner = 'nam20485',
  [string]$Template = 'nam20485/ai-new-app-template',

  [switch]$Public = $true,
  [switch]$Private,

  [string]$License = 'agpl-3.0',
  [string]$DefaultBranch = 'development',

  [string]$WorkspaceRoot,

  [string[]]$MilestoneTitles = @(
    'Phase 1: Foundation',
    'Phase 2: Core Features',
    'Phase 3: Validation'
  ),
  [string[]]$MilestoneDueDates = @(
    '2025-09-15',
    '2025-10-15',
    '2025-11-15'
  ),

  [switch]$SkipProject,
  [switch]$SkipRepoCreate,
  [switch]$SkipClone,
  [switch]$SkipDocs,
  [switch]$SkipLabels,
  [switch]$SkipMilestones,
  [switch]$SkipInitTemplateScript,

  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

function Write-Info([string]$Message) { Write-Host $Message -ForegroundColor Cyan }
function Write-Warn([string]$Message) { Write-Host $Message -ForegroundColor Yellow }
function Write-Ok([string]$Message) { Write-Host $Message -ForegroundColor Green }
function Write-Err([string]$Message) { Write-Host $Message -ForegroundColor Red }

function Invoke-ExternalCommand {
  param(
    [Parameter(Mandatory = $true)][string]$FilePath,
    [string[]]$ArgumentList = @(),
    [switch]$AllowFail
  )
  $cmd = "$FilePath $($ArgumentList -join ' ')"
  Write-Info ">> $cmd"
  if ($DryRun) { return @{ ExitCode = 0; Output = @('<dry-run>'); Error = @() } }
  $output = & $FilePath @ArgumentList 2>&1
  $exit = $LASTEXITCODE
  if ($exit -ne 0 -and -not $AllowFail) {
    Write-Err ("Command failed ({0}): {1}`n{2}" -f $exit, $FilePath, ($output -join "`n"))
    throw "External command failed with exit code $exit"
  }
  return @{ ExitCode = $exit; Output = $output; Error = @() }
}

function Get-WorkspaceRoot {
  param([string]$Explicit)
  if ($Explicit) { return (Resolve-Path -LiteralPath $Explicit).Path }
  try {
    $gitRoot = (& git rev-parse --show-toplevel 2>$null)
    if ($LASTEXITCODE -eq 0 -and $gitRoot) { return $gitRoot.Trim() }
  } catch {}
  return (Resolve-Path '.').Path
}

function Ensure-Tool {
  param([Parameter(Mandatory=$true)][string]$Name)
  $found = Get-Command $Name -ErrorAction SilentlyContinue
  if (-not $found) { throw "Required tool not found on PATH: $Name" }
}

function Ensure-GhAuth {
  $res = Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('auth','status') -AllowFail
  if ($res.ExitCode -ne 0) {
    Write-Warn 'GitHub CLI not authenticated. Initiating gh auth login so the user can complete prompts...'
    Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('auth','login') -AllowFail | Out-Null
  }
}

function Ensure-OwnerProject {
  param([string]$Owner,[string]$Title)
  Write-Info "Ensuring owner-level project exists: $Owner / $Title"
  $list = Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('project','list','--owner', $Owner, '--limit','200','--format','json')
  $projects = @()
  if ($list.Output) { try { $projects = ($list.Output -join "`n") | ConvertFrom-Json } catch { $projects=@() } }
  $existing = $projects | Where-Object { $_.title -eq $Title } | Select-Object -First 1
  if ($existing) {
    Write-Ok ("Project exists: #{0} -> {1}" -f $existing.number, $existing.shortDescriptionURL)
    return
  }
  $created = Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('project','create','--owner', $Owner, '--title', $Title, '--format','json')
  if (-not $DryRun) {
    $obj = ($created.Output -join "`n") | ConvertFrom-Json
    Write-Ok ("Created project #{0} -> {1}" -f $obj.number, $obj.url)
  } else {
    Write-Ok "[dry-run] Would create project $Owner/$Title"
  }
}

function Ensure-Repository {
  param([string]$Owner,[string]$Name,[string]$Template,[switch]$IsPublic,[string]$License)
  Write-Info "Ensuring repository exists: $Owner/$Name"
  $view = Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('repo','view',"$Owner/$Name") -AllowFail
  if ($view.ExitCode -eq 0) {
    Write-Ok "Repository exists: $Owner/$Name"
    return
  }
  $args = @('repo','create',"$Owner/$Name",'--template', $Template)
  if ($IsPublic) { $args += '--public' } else { $args += '--private' }
  if ($License) { $args += @('--license', $License) }
  Invoke-ExternalCommand -FilePath 'gh' -ArgumentList $args | Out-Null
  Write-Ok "Repository created: $Owner/$Name"
}

function Ensure-DefaultBranch {
  param([string]$Owner,[string]$Name,[string]$Branch)
  Write-Info "Ensuring default branch for $Owner/$Name is '$Branch'"
  Invoke-ExternalCommand -FilePath 'gh' -ArgumentList @('repo','edit',"$Owner/$Name",'--default-branch', $Branch) | Out-Null
  Write-Ok "Default branch ensured: $Branch"
}

function Get-DestPath {
  param([string]$WorkspaceRoot,[string]$RepoName)
  $dynamicWorkflows = Join-Path $WorkspaceRoot 'dynamic_workflows'
  if (-not (Test-Path -LiteralPath $dynamicWorkflows)) {
    New-Item -ItemType Directory -Path $dynamicWorkflows | Out-Null
  }
  $dest = Join-Path $dynamicWorkflows $RepoName
  if (-not ($dest.ToLowerInvariant().StartsWith($dynamicWorkflows.ToLowerInvariant()))) {
    throw "Refusing to clone outside workspace dynamic_workflows root: $dest"
  }
  return $dest
}

function Ensure-Clone {
  param([string]$Owner,[string]$Name,[string]$Dest)
  Write-Info "Ensuring clone at: $Dest"
  if (Test-Path -LiteralPath $Dest) {
    Write-Ok "Folder exists: $Dest (skipping clone)"
    return
  }
  New-Item -ItemType Directory -Path $Dest | Out-Null
  Invoke-ExternalCommand -FilePath 'git' -ArgumentList @('clone',"https://github.com/$Owner/$Name.git", $Dest) | Out-Null
  Write-Ok "Cloned into: $Dest"
}

function Copy-SeedDocs {
  param([string]$WorkspaceRoot,[string]$Dest)
  $srcDir = Join-Path $WorkspaceRoot 'docs/advanced_memory'
  $destDocs = Join-Path $Dest 'docs'
  if (-not (Test-Path -LiteralPath $srcDir)) {
    Write-Warn "Source docs folder not found: $srcDir (skipping copy)"
    return
  }
  if (-not (Test-Path -LiteralPath $destDocs)) {
    New-Item -ItemType Directory -Path $destDocs | Out-Null
  }
  $specificFiles = @('Advanced Memory .NET - Dev Plan.md','index.html')
  $copied = $false
  foreach ($f in $specificFiles) {
    $src = Join-Path $srcDir $f
    if (Test-Path -LiteralPath $src) {
      $dst = Join-Path $destDocs $f
      Write-Info "Copying: $src -> $dst"
      if (-not $DryRun) { Copy-Item -LiteralPath $src -Destination $dst -Force }
      $copied = $true
    } else {
      Write-Warn "Missing source file: $src"
    }
  }
  if (-not $copied) {
    # Fallback: copy all md and html
    $patterns = @('*.md','*.html','*.htm')
    foreach ($pat in $patterns) {
      $fileMatches = Get-ChildItem -LiteralPath $srcDir -Filter $pat -File -ErrorAction SilentlyContinue
      foreach ($m in $fileMatches) {
        $dst = Join-Path $destDocs $m.Name
        Write-Info ("Copying (fallback): {0} -> {1}" -f $m.FullName, $dst)
        if (-not $DryRun) { Copy-Item -LiteralPath $m.FullName -Destination $dst -Force }
        $copied = $true
      }
    }
    if (-not $copied) { Write-Warn "No seed docs found to copy in $srcDir" }
  }
  Write-Ok "Seed docs copy complete"
}

function Invoke-ImportLabels {
  param([string]$RepoRoot,[string]$Owner,[string]$Repo)
  $scriptLocal = Join-Path $RepoRoot 'scripts/import-labels.ps1'
  $scriptFallback = Join-Path (Get-WorkspaceRoot $null) 'scripts/import-labels.ps1'
  $scriptPath = if (Test-Path -LiteralPath $scriptLocal) { $scriptLocal } elseif (Test-Path -LiteralPath $scriptFallback) { $scriptFallback } else { $null }
  if (-not $scriptPath) { Write-Warn 'import-labels.ps1 not found. Skipping labels import.'; return }
  $labelsJson = Join-Path $RepoRoot '.labels.json'
  if (-not (Test-Path -LiteralPath $labelsJson)) {
    Write-Warn "Labels file not found: $labelsJson (skipping)"
    return
  }
  Write-Info "Importing labels using: $scriptPath"
  if (-not $DryRun) {
    & pwsh -NoProfile -File $scriptPath -Owner $Owner -Repo $Repo -LabelsPath $labelsJson
  } else {
    Write-Ok "[dry-run] Would import labels for $Owner/$Repo from $labelsJson"
  }
}

function Invoke-CreateMilestones {
  param([string]$RepoRoot,[string]$Owner,[string]$Repo,[string[]]$Titles,[string[]]$DueDates)
  if ($Titles.Count -ne $DueDates.Count) {
    throw "MilestoneTitles and MilestoneDueDates must have same length"
  }
  $scriptLocal = Join-Path $RepoRoot 'scripts/create-milestones.ps1'
  $scriptFallback = Join-Path (Get-WorkspaceRoot $null) 'scripts/create-milestones.ps1'
  $scriptPath = if (Test-Path -LiteralPath $scriptLocal) { $scriptLocal } elseif (Test-Path -LiteralPath $scriptFallback) { $scriptFallback } else { $null }
  if (-not $scriptPath) { Write-Warn 'create-milestones.ps1 not found. Skipping milestones.'; return }
  Write-Info "Creating milestones using: $scriptPath"
  if (-not $DryRun) {
    & pwsh -NoProfile -File $scriptPath -Owner $Owner -Repo $Repo -Titles $Titles -DueDates $DueDates -SkipExisting
  } else {
    Write-Ok "[dry-run] Would create milestones for ${Owner}/${Repo}: $($Titles -join ', ')"
  }
}

function Invoke-CommitAndPush {
  param([string]$RepoRoot,[string]$Branch)
  Write-Info "Committing and pushing changes on branch '$Branch'"
  Invoke-ExternalCommand -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'checkout','-B', $Branch) | Out-Null
  Invoke-ExternalCommand -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'add','.') | Out-Null
  $commit = Invoke-ExternalCommand -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'commit','-m','Seed docs, labels, milestones, workspace/devcontainer rename') -AllowFail
  if ($commit.ExitCode -ne 0) {
    $msg = ($commit.Output -join ' ')
    if ($msg -match 'nothing to commit') {
      Write-Warn 'Nothing to commit (working tree clean)'
    } else {
      throw 'git commit failed'
    }
  }
  Invoke-ExternalCommand -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'push','-u','origin', $Branch) | Out-Null
  Write-Ok 'Pushed changes'
}

function Invoke-InitTemplateScript {
  param([string]$RepoRoot,[string]$RepoName)
  $scriptLocal = Join-Path $RepoRoot 'scripts/init-template-repo.ps1'
  $scriptFallback = Join-Path (Get-WorkspaceRoot $null) 'scripts/init-template-repo.ps1'
  $scriptPath = if (Test-Path -LiteralPath $scriptLocal) { $scriptLocal } elseif (Test-Path -LiteralPath $scriptFallback) { $scriptFallback } else { $null }
  if (-not $scriptPath) { Write-Warn 'init-template-repo.ps1 not found. Skipping post-clone init.'; return }
  Write-Info "Running init-template-repo.ps1 using: $scriptPath"
  if (-not $DryRun) {
    & pwsh -NoProfile -File $scriptPath -RepoPath $RepoRoot -RepoName $RepoName
  } else {
    Write-Ok "[dry-run] Would run init-template-repo.ps1 for $RepoName at $RepoRoot"
  }
}

Write-Host '=== Initiate New Repository (dynamic workflow) ===' -ForegroundColor Magenta

# Dot-source common auth helper if present
$commonAuth = Join-Path $PSScriptRoot 'common-auth.ps1'
if (Test-Path -LiteralPath $commonAuth) { . $commonAuth }

# Preconditions
Ensure-Tool 'git'
Ensure-Tool 'gh'
if (Get-Command Initialize-GitHubAuth -ErrorAction SilentlyContinue) { Initialize-GitHubAuth } else { Ensure-GhAuth }

$wsRoot = Get-WorkspaceRoot -Explicit $WorkspaceRoot
Write-Info ("Workspace root: {0}" -f $wsRoot)

$dest = Get-DestPath -WorkspaceRoot $wsRoot -RepoName $RepoName
Write-Info ("Destination path: {0}" -f $dest)

# Create owner-level project
if (-not $SkipProject) { Ensure-OwnerProject -Owner $Owner -Title $RepoName }
else { Write-Warn 'SkipProject set: skipping owner-level project creation' }

# Create repository from template
if ($Private) { $Public = $false }
if (-not $SkipRepoCreate) { Ensure-Repository -Owner $Owner -Name $RepoName -Template $Template -IsPublic:$Public -License $License }
else { Write-Warn 'SkipRepoCreate set: skipping repo creation' }

# Ensure default branch
Ensure-DefaultBranch -Owner $Owner -Name $RepoName -Branch $DefaultBranch

# Clone repository
if (-not $SkipClone) { Ensure-Clone -Owner $Owner -Name $RepoName -Dest $dest }
else { Write-Warn 'SkipClone set: skipping clone' }

# Copy seed docs
if (-not $SkipDocs) { Copy-SeedDocs -WorkspaceRoot $wsRoot -Dest $dest }
else { Write-Warn 'SkipDocs set: skipping docs copy' }

# Import labels
if (-not $SkipLabels) { Invoke-ImportLabels -RepoRoot $dest -Owner $Owner -Repo $RepoName }
else { Write-Warn 'SkipLabels set: skipping labels import' }

# Create milestones
if (-not $SkipMilestones) { Invoke-CreateMilestones -RepoRoot $dest -Owner $Owner -Repo $RepoName -Titles $MilestoneTitles -DueDates $MilestoneDueDates }
else { Write-Warn 'SkipMilestones set: skipping milestones' }

# Commit and push
Invoke-CommitAndPush -RepoRoot $dest -Branch $DefaultBranch

# Post-clone initialization
if (-not $SkipInitTemplateScript) { Invoke-InitTemplateScript -RepoRoot $dest -RepoName $RepoName }
else { Write-Warn 'SkipInitTemplateScript set: skipping post-clone init' }

Write-Ok 'All done.'
