#!/usr/bin/env pwsh
<#!
PR Review Threads helper
- Summarize unresolved review threads for a PR
- Optional dry run to list what would be resolved
- Optional resolve of all unresolved threads
- Optional reply to the latest comment in each thread before resolving

Requires: GitHub CLI (gh) authenticated with repo scope
!#>

[CmdletBinding()]
param(
    [string]$Owner = "nam20485",
    [string]$Repo = "agent-instructions",
    [int]$PullRequestNumber = 9,
    # Selection controls
    [string]$ThreadId,
    [string]$Path,
    [string]$BodyContains,
    [switch]$Interactive,
    [switch]$AutoResolve,
    [switch]$NoResolve,
    [switch]$DryRun,
    [switch]$VerboseLogging,
    [string]$ReplyEach # Optional message to post as a reply to the latest comment in each unresolved thread
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-Tool {
    param([Parameter(Mandatory)][string]$Name)
    try { Get-Command $Name -ErrorAction Stop | Out-Null; $true } catch { $false }
}

if (-not (Test-Tool gh)) {
    throw "GitHub CLI (gh) not found. Install from https://cli.github.com and authenticate via 'gh auth login'."
}

if ($VerboseLogging) { gh auth status || Write-Host "(gh auth status failed to display; continuing)" -ForegroundColor DarkYellow }

function Invoke-GHGraphQLQuery {
    param(
        [Parameter(Mandatory)][string]$Query,
        [hashtable]$Vars
    )
    $ghArgs = @('api','graphql','-f',"query=$Query")
    foreach ($kv in $Vars.GetEnumerator()) {
        $k = $kv.Key
        $v = $kv.Value
        if ($v -is [int] -or $v -is [long] -or $v -is [double] -or $v -is [decimal] -or $v -is [bool]) {
            # Use -F to pass raw JSON value for correct GraphQL typing
            $ghArgs += @('-F',"$k=$v")
        } else {
            $ghArgs += @('-f',"$k=$v")
        }
    }
    if ($VerboseLogging) { Write-Host "gh $($ghArgs -join ' ')" -ForegroundColor DarkGray }
    $out = gh @ghArgs
    if ([string]::IsNullOrWhiteSpace($out)) {
        throw "GraphQL returned empty response"
    }
    try {
        $json = $out | ConvertFrom-Json
    } catch {
        if ($VerboseLogging) { Write-Host "Non-JSON response: $out" -ForegroundColor DarkYellow }
        throw "Failed to parse GraphQL JSON: $($_.Exception.Message)"
    }
    $hasErrors = $false
    $errMsgs = @()
    $hasErrorsProp = $false
    try {
        $null = $json.errors
        $hasErrorsProp = $true
    } catch { $hasErrorsProp = $false }
    if ($hasErrorsProp -and $null -ne $json.errors) {
        $errs = $json.errors
        if ($errs -is [array]) { $errMsgs = $errs | ForEach-Object { $_.message } }
        elseif ($errs) { $errMsgs = @($errs.message) }
        if ($errMsgs.Count -gt 0) { $hasErrors = $true }
    }
    if ($hasErrors) { throw "GraphQL error: $(( $errMsgs -join '; '))" }
    return $json
}

function Get-UnresolvedReviewThreads {
    param([string]$Owner,[string]$Repo,[int]$PR)
        $q = @'
query($owner:String!, $repo:String!, $number:Int!) {
  repository(owner: $owner, name: $repo) {
    name
    pullRequest(number: $number) {
      number
      url
            reviewThreads(first: 100) {
        nodes {
          id
          isResolved
          isCollapsed
          path
          comments(first: 100) {
            nodes {
              id
              databaseId
              author { login }
              body
              createdAt
              isMinimized
              minimizedReason
            }
          }
        }
      }
    }
  }
}
'@
    $vars = @{ owner=$Owner; repo=$Repo; number=$PR }
    $resp = Invoke-GHGraphQLQuery -Query $q -Vars $vars
    $threads = @()
    if ($null -eq $resp -or $null -eq $resp.data -or $null -eq $resp.data.repository -or $null -eq $resp.data.repository.pullRequest) {
        throw "GraphQL response missing expected data for repository/pullRequest."
    }
    $nodes = $resp.data.repository.pullRequest.reviewThreads.nodes
    if ($nodes) {
        foreach ($n in $nodes) {
            if ($n.isResolved) { continue }
            $comments = @()
            if ($n.comments.nodes) { $comments = @($n.comments.nodes) }
            $threads += [pscustomobject]@{
                ThreadId   = $n.id
                Path       = $n.path
                IsResolved = $n.isResolved
                Comments   = $comments
            }
        }
    }
    return ,$threads
}

function Get-LastComment {
    param($Thread)
    if ($Thread -and $Thread.Comments -and $Thread.Comments.Count -gt 0) {
        return $Thread.Comments[-1]
    }
    return $null
}

function Resolve-ReviewThread {
    param([Parameter(Mandatory)][string]$ThreadId)
    $m = @'
mutation($threadId:ID!) {
  resolveReviewThread(input:{threadId:$threadId}) {
    thread { id isResolved }
  }
}
'@
    $resp = Invoke-GHGraphQLQuery -Query $m -Vars @{ threadId=$ThreadId }
    return $resp
}

function Send-ReplyToReviewComment {
    param(
        [Parameter(Mandatory)][string]$Owner,
        [Parameter(Mandatory)][string]$Repo,
        [Parameter(Mandatory)][int]$PR,
        [Parameter(Mandatory)][string]$CommentId,
        [Parameter(Mandatory)][string]$Body
    )
    $ghArgs = @(
        'api','-X','POST',
        "repos/$Owner/$Repo/pulls/comments/$CommentId/replies",
        '-f',"body=$Body"
    )
    if ($VerboseLogging) { Write-Host "gh $($ghArgs -join ' ')" -ForegroundColor DarkGray }
    gh @ghArgs | Out-Null
}

$threads = Get-UnresolvedReviewThreads -Owner $Owner -Repo $Repo -PR $PullRequestNumber

# Apply selectors (by ThreadId, Path, BodyContains)
$selected = $threads
if ($PSBoundParameters.ContainsKey('ThreadId') -and $ThreadId) {
    $selected = $selected | Where-Object { $_.ThreadId -eq $ThreadId }
}
if ($PSBoundParameters.ContainsKey('Path') -and $Path) {
    $selected = $selected | Where-Object { $_.Path -like "*${Path}*" }
}
if ($PSBoundParameters.ContainsKey('BodyContains') -and $BodyContains) {
    $selected = $selected | Where-Object {
        $lc = Get-LastComment $_
        if ($null -eq $lc -or [string]::IsNullOrWhiteSpace($lc.body)) { return $false }
        return ($lc.body -match [regex]::Escape($BodyContains))
    }
}

$unresolvedCount = ($selected | Measure-Object).Count

# Build markdown summary
$summaryPath = Join-Path (Get-Location) 'pr-review-threads-summary.md'
$md = @()
$md += "# PR #$PullRequestNumber Unresolved Review Threads"
$md += ""
$md += "- Repository: $Owner/$Repo"
$md += "- Count: $unresolvedCount"
$md += ""
$bt = [char]96
foreach ($t in $selected) {
    $last = Get-LastComment $t
    $snippet = if ($last) { ($last.body -replace "\r?\n"," ") } else { '' }
    if ($snippet.Length -gt 100) { $snippet = $snippet.Substring(0,100) + '…' }
    $lastBy = if ($last -and $last.author) { $last.author.login } else { '' }
    $line = "- ThreadId: $bt$($t.ThreadId)$bt Path: $bt$($t.Path)$bt LastBy: $bt$lastBy$bt Snippet: $snippet"
    $md += $line
}
$mdText = ($md -join [Environment]::NewLine)
$mdText | Set-Content -Encoding UTF8 $summaryPath
Write-Host "Wrote summary -> $summaryPath" -ForegroundColor Cyan
Write-Host $mdText

if ($NoResolve) {
    Write-Host "NoResolve flag set; exiting without resolving threads." -ForegroundColor Yellow
    exit 0
}

if ($DryRun) {
    Write-Host "DryRun: Would resolve $unresolvedCount thread(s)." -ForegroundColor Yellow
    foreach ($t in $selected) { Write-Host "  - $($t.ThreadId) ($($t.Path))" }
    exit 0
}

if ($unresolvedCount -eq 0) {
    Write-Host "No unresolved review threads found for PR #$PullRequestNumber." -ForegroundColor Green
    exit 0
}

# Helper to reply then resolve a single thread
function Resolve-OneThread {
    param($Thread)
    $last = Get-LastComment $Thread
    if (-not [string]::IsNullOrWhiteSpace($ReplyEach) -and $last -and $last.databaseId) {
        Write-Host "Replying to comment $($last.databaseId) on $($Thread.Path)" -ForegroundColor DarkCyan
        Send-ReplyToReviewComment -Owner $Owner -Repo $Repo -PR $PullRequestNumber -CommentId $last.databaseId -Body $ReplyEach
    }
    Write-Host "Resolving thread: $($Thread.ThreadId) ($($Thread.Path))"
    Resolve-ReviewThread -ThreadId $Thread.ThreadId | Out-Null
}

$resolved = 0
if ($Interactive -or (-not $AutoResolve -and $unresolvedCount -gt 1)) {
    foreach ($t in $selected) {
        $last = Get-LastComment $t
        $snippet = if ($last) { ($last.body -replace "\r?\n"," ") } else { '' }
        if ($snippet.Length -gt 140) { $snippet = $snippet.Substring(0,140) + '…' }
        Write-Host "\nThreadId: $($t.ThreadId)" -ForegroundColor Cyan
        Write-Host "Path    : $($t.Path)"
        if ($last) { Write-Host "LastBy  : $($last.author.login) @ $($last.createdAt)" }
        if ($snippet) { Write-Host "Snippet : $snippet" }
        $ans = Read-Host "Resolve this thread now? (y/N)"
        if ($ans -match '^(y|yes)$') {
            try { Resolve-OneThread -Thread $t; $resolved++ } catch { Write-Warning "Failed to resolve $($t.ThreadId): $($_.Exception.Message)" }
        } else {
            Write-Host "Skipped $($t.ThreadId)"
        }
    }
} else {
    foreach ($t in $selected) {
        try { Resolve-OneThread -Thread $t; $resolved++ } catch { Write-Warning "Failed to resolve $($t.ThreadId): $($_.Exception.Message)" }
    }
}

Write-Host "Resolved $resolved thread(s)." -ForegroundColor Green
