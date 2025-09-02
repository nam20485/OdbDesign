param(
  [string]$Owner = 'nam20485',
  [string]$Repo = 'agent-instructions',
  [string]$Branch = 'main'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-GitHubApiJson {
  param(
    [Parameter(Mandatory=$true)][string]$Url
  )
  $headers = @{ 'User-Agent' = 'workflow-launch2-index-updater' }
  if ($env:GITHUB_TOKEN) { $headers['Authorization'] = "Bearer $($env:GITHUB_TOKEN)" }
  return Invoke-RestMethod -Method GET -Uri $Url -Headers $headers -ErrorAction Stop
}

function Get-DirectoryMarkdownFiles {
  param(
    [Parameter(Mandatory=$true)][string]$Path
  )
  $api = "https://api.github.com/repos/$Owner/$Repo/contents/${Path}?ref=$Branch"
  $items = Invoke-GitHubApiJson -Url $api
  # Filter markdown files only
  $files = @($items | Where-Object { $_.type -eq 'file' -and $_.name.ToLower().EndsWith('.md') })
  # Return ordered by name
  return $files | Sort-Object -Property name
}

function Build-AssignmentsIndexContent {
  param(
    [Parameter(Mandatory=$true)][array]$AssignmentFiles,
    [Parameter(Mandatory=$true)][string]$DirPath
  )
  $lines = @()
  $dirWithSlash = ($DirPath.TrimEnd('/') + '/')
  $lines += '# Workflow Assignments Index'
  $lines += ''
  $lines += "Repository: $Owner/$Repo"
  $lines += "Full repo URL: https://github.com/$Owner/$Repo"
  $lines += "Branch: $Branch"
  $lines += "Directory: $dirWithSlash"
  $lines += ''
  $lines += 'Listed beklow are all of the active workflow assignments and their paths.'
  $lines += ''
  $lines += 'Agents MUST resolve workflow assignments (by shortId) from the remote canonical repository. Do not use local mirrors.'
  $lines += ''
  $lines += '## Location of Remote Repository'
  $lines += ''
  $lines += "- Repository: $Owner/$Repo"
  $lines += "- Branch: $Branch"
  $lines += ('- Directory: `{0}`' -f $dirWithSlash)
  $lines += ''
  $lines += '## Workflow Assignments '
  $lines += ''

  foreach ($f in $AssignmentFiles) {
    $shortId = [System.IO.Path]::GetFileNameWithoutExtension($f.name)
    $ui = "https://github.com/$Owner/$Repo/blob/$Branch/$($f.path)"
    $raw = "https://raw.githubusercontent.com/$Owner/$Repo/$Branch/$($f.path)"
    $lines += "#### $shortId"
    $lines += ''
    $lines += "- shortId: $shortId"
    $lines += ''
    $lines += "- GitHub UI: [$shortId]($ui)"
    $lines += "- Raw URL:   [$shortId]($raw)"
    $lines += "- Canonical file: ``$($f.path)``"
    $lines += ''
  }

  return ($lines -join "`r`n")
}

function Build-DynamicWorkflowsIndexContent {
  param(
    [Parameter(Mandatory=$true)][array]$WorkflowFiles,
    [Parameter(Mandatory=$true)][string]$DirPath
  )
  $lines = @()
  $dirWithSlash = ($DirPath.TrimEnd('/') + '/')
  $lines += '# Dynamic Workflows Index'
  $lines += ''
  $lines += "Repository: $Owner/$Repo"
  $lines += "Full repo URL: https://github.com/$Owner/$Repo"
  $lines += "Branch: $Branch"
  $lines += "Directory: $dirWithSlash"
  $lines += ''
  $lines += 'Listed beklow are all of the active dynamic workflows and their paths.'
  $lines += ''
  $lines += 'Agents MUST resolve dynamic workflows from the remote canonical repository. Do not use local mirrors.'
  $lines += ''
  $lines += '## Location of Remote Repository'
  $lines += ''
  $lines += "- Repository: $Owner/$Repo"
  $lines += "- Branch: $Branch"
  $lines += ('- Directory: `{0}`' -f $dirWithSlash)
  $lines += ''
  $lines += '## Dynamic Workflows '
  $lines += ''

  foreach ($f in $WorkflowFiles) {
    $shortId = [System.IO.Path]::GetFileNameWithoutExtension($f.name)
    $ui = "https://github.com/$Owner/$Repo/blob/$Branch/$($f.path)"
    $raw = "https://raw.githubusercontent.com/$Owner/$Repo/$Branch/$($f.path)"
    $lines += "#### $shortId"
    $lines += ''
    $lines += "- shortId: $shortId"
    $lines += ''
    $lines += "- GitHub UI: [$shortId]($ui)"
    $lines += "- Raw URL:   [$shortId]($raw)"
    $lines += "- Canonical file: ``$($f.path)``"
    $lines += ''
  }

  return ($lines -join "`r`n")
}

# Resolve paths
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$assignmentsIndexPath = Join-Path $repoRoot 'local_ai_instruction_modules\ai-workflow-assignments.md'
$workflowsIndexPath   = Join-Path $repoRoot 'local_ai_instruction_modules\ai-dynamic-workflows.md'

# Fetch remote file lists
$assignmentsDir = 'ai_instruction_modules/ai-workflow-assignments'
$workflowsDir   = 'ai_instruction_modules/ai-workflow-assignments/dynamic-workflows'

$assignmentFiles = Get-DirectoryMarkdownFiles -Path $assignmentsDir
# Exclude files that are part of sub-directories, but API already scopes by dir; skip none.
$workflowFiles   = Get-DirectoryMarkdownFiles -Path $workflowsDir

# Build contents
$assignmentsContent = Build-AssignmentsIndexContent -AssignmentFiles $assignmentFiles -DirPath $assignmentsDir
$workflowsContent   = Build-DynamicWorkflowsIndexContent -WorkflowFiles $workflowFiles -DirPath $workflowsDir

# Write only if changed
function Write-IfChanged {
  param(
    [Parameter(Mandatory=$true)][string]$Path,
    [Parameter(Mandatory=$true)][string]$Content
  )
  $existing = if (Test-Path $Path) { Get-Content -Path $Path -Raw -ErrorAction Stop } else { '' }
  if ($existing -ne $Content) {
    $Content | Out-File -FilePath $Path -Encoding UTF8
    Write-Host "Updated: $Path" -ForegroundColor Green
  } else {
    Write-Host "No changes: $Path" -ForegroundColor Yellow
  }
}

Write-IfChanged -Path $assignmentsIndexPath -Content $assignmentsContent
Write-IfChanged -Path $workflowsIndexPath   -Content $workflowsContent
