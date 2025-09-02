# Common authentication helpers for GitHub CLI
# Dot-source this file from scripts that need to ensure authentication.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Initialize-GitHubAuth {
	[CmdletBinding()]
	param(
		[switch]$DryRun
	)
	# Verify gh is available
	if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
		throw 'Required tool not found on PATH: gh'
	}
	# Check auth status; if not authenticated, initiate login so user can complete prompts
	& gh auth status 2>$null | Out-Null
	$code = $LASTEXITCODE
	if ($code -ne 0) {
		Write-Warning 'GitHub CLI not authenticated. Initiating gh auth login so the user can complete prompts...'
		if ($DryRun) {
			Write-Host '[dry-run] Would run: gh auth login' -ForegroundColor Yellow
		} else {
			& gh auth login | Out-Null
		}
	}
}
