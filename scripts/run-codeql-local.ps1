#!/usr/bin/env pwsh
#
# Local CodeQL Analysis Script for OdbDesign
# This script helps you run CodeQL analysis locally before pushing to GitHub
#

param(
    [string]$DatabasePath = "./codeql-db",
    [string]$OutputPath = "./codeql-results",
    [switch]$Clean,
    [switch]$SkipBuild
)

# Colors for output
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-ColorOutput {
    param($Color, $Message)
    Write-Host "${Color}${Message}${Reset}"
}

# Check if CodeQL CLI is installed
if (-not (Get-Command "codeql" -ErrorAction SilentlyContinue)) {
    Write-ColorOutput $Red "ERROR: CodeQL CLI not found. Please install from https://github.com/github/codeql-cli-binaries"
    exit 1
}

Write-ColorOutput $Blue "🔍 OdbDesign CodeQL Local Analysis"
Write-ColorOutput $Blue "================================="

# Clean previous results if requested
if ($Clean -and (Test-Path $DatabasePath)) {
    Write-ColorOutput $Yellow "🧹 Cleaning previous database..."
    Remove-Item $DatabasePath -Recurse -Force
}

if ($Clean -and (Test-Path $OutputPath)) {
    Write-ColorOutput $Yellow "🧹 Cleaning previous results..."
    Remove-Item $OutputPath -Recurse -Force
}

# Create CodeQL database
if (-not (Test-Path $DatabasePath) -or $Clean) {
    Write-ColorOutput $Yellow "📊 Creating CodeQL database..."
    
    if ($SkipBuild) {
        # Create database without build (for header-only libraries)
        $createCmd = "codeql database create `"$DatabasePath`" --language=cpp --source-root=. --overwrite"
    } else {
        # Create database with build
        $buildCmd = "cmake --preset linux-release && cmake --build --preset linux-release"
        $createCmd = "codeql database create `"$DatabasePath`" --language=cpp --command=`"$buildCmd`" --source-root=. --overwrite"
    }
    
    Write-ColorOutput $Blue "Running: $createCmd"
    Invoke-Expression $createCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-ColorOutput $Red "❌ Database creation failed!"
        exit $LASTEXITCODE
    }
    Write-ColorOutput $Green "✅ Database created successfully"
}

# Run analysis
Write-ColorOutput $Yellow "🔍 Running CodeQL analysis..."
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

# Analyze with custom config
$analyzeCmd = "codeql database analyze `"$DatabasePath`" .github/codeql/cpp-queries/odbdesign-suite.qls --format=sarif-latest --output=`"$OutputPath/custom-results.sarif`""
Write-ColorOutput $Blue "Running: $analyzeCmd"
Invoke-Expression $analyzeCmd

if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput $Red "❌ Custom analysis failed!"
    exit $LASTEXITCODE
}

# Analyze with security suite  
$securityCmd = "codeql database analyze `"$DatabasePath`" codeql/cpp-queries:Security --format=sarif-latest --output=`"$OutputPath/security-results.sarif`""
Write-ColorOutput $Blue "Running: $securityCmd"
Invoke-Expression $securityCmd

if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput $Red "❌ Security analysis failed!"
    exit $LASTEXITCODE
}

Write-ColorOutput $Green "✅ Analysis completed successfully!"
Write-ColorOutput $Blue "📁 Results saved to: $OutputPath"

# Display summary
$customResults = Get-Content "$OutputPath/custom-results.sarif" | ConvertFrom-Json
$securityResults = Get-Content "$OutputPath/security-results.sarif" | ConvertFrom-Json

$customCount = $customResults.runs[0].results.Count
$securityCount = $securityResults.runs[0].results.Count

Write-ColorOutput $Blue "📊 Analysis Summary:"
Write-ColorOutput $Yellow "   Custom queries: $customCount findings"
Write-ColorOutput $Yellow "   Security queries: $securityCount findings"
Write-ColorOutput $Yellow "   Total: $($customCount + $securityCount) findings"

if ($customCount -gt 0 -or $securityCount -gt 0) {
    Write-ColorOutput $Yellow "⚠️  Security findings detected! Review results before committing."
} else {
    Write-ColorOutput $Green "🎉 No security issues found!"
}

Write-ColorOutput $Blue "💡 Tip: Use 'codeql bqrs decode' to view detailed results or import SARIF files into VS Code"
