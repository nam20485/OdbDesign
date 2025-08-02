# Environment Setup Scripts Documentation

This directory contains scripts for setting up and validating a development environment with Node.js, npm, Firebase CLI, Google Cloud CLI, and GitHub CLI.

## Scripts Overview

### 1. install-environment.ps1
**Purpose**: Installs all development tools cross-platform (Windows/Linux)

**Features**:
- Cross-platform PowerShell Core script
- Fail-fast error handling with comprehensive error checking
- Idempotent (safe to run multiple times)
- Installs: NVM, Node.js LTS, npm, Firebase CLI v14.3.1, Google Cloud CLI, GitHub CLI
- Updates system PATH and current session PATH

**Usage**:
```powershell
# Run installation
pwsh -File "install-environment.ps1"
```

### 2. validate-environment.ps1
**Purpose**: Validates that all development tools are properly installed and accessible

**Features**:
- Comprehensive tool verification with version checking
- Colored output for easy reading (✅ PASS / ❌ FAIL)
- PATH verification and system compatibility checks
- Detailed error reporting
- Exit codes for automation (0 = success, 1 = failure)

**Usage**:
```powershell
# Validate current environment
pwsh -File "validate-environment.ps1"
```

**Sample Output**:
```
=== Development Environment Validation ===
Node.js (node)
  Status: ✅ PASS
  Version: v22.16.0

Firebase CLI (firebase)
  Status: ✅ PASS
  Version: 14.3.1
  Expected: 14\.3\.1

Tools validated: 6/6
🎉 All development tools are properly installed and accessible!
```

### 3. test-environment.ps1
**Purpose**: Tests the installation script in a simulated clean environment

**Features**:
- Backup and restore environment functionality
- Clean environment simulation (removes tool paths)
- Comprehensive testing workflow (pre-install → install → post-install validation)
- Detailed logging with timestamps
- Interactive restore prompts

**Usage**:
```powershell
# Test installation with clean environment simulation
pwsh -File "test-environment.ps1" -SimulateClean

# Test without environment backup (faster)
pwsh -File "test-environment.ps1" -SimulateClean -SkipBackup

# Restore environment from previous test
pwsh -File "test-environment.ps1" -RestoreOnly
```

## Prerequisites

### All Platforms
- **PowerShell Core 7.0+** (`pwsh` command)
- Internet connection for downloading installers

### Windows Specific
- **Chocolatey** (for NVM and GitHub CLI installation)
- **Administrator privileges** (recommended for system-wide installations)

### Linux Specific
- **curl** (usually pre-installed)
- **sudo access** (for package manager operations)

## Installation Process

1. **Check Prerequisites**: Ensure PowerShell Core is installed
2. **Run Installation**: Execute `install-environment.ps1`
3. **Validate Installation**: Run `validate-environment.ps1`
4. **Test (Optional)**: Run `test-environment.ps1` for comprehensive testing

### Quick Start
```powershell
# Navigate to scripts directory
cd "path\to\AgentAsAService\scripts"

# Install tools
pwsh -File "install-environment.ps1"

# Validate installation
pwsh -File "validate-environment.ps1"
```

## Troubleshooting

### Common Issues

#### 1. Tools not accessible after installation
**Symptom**: Installation succeeds but commands not found
**Solution**: Restart terminal or VS Code to pick up PATH changes
```powershell
# Or manually update current session PATH (Windows)
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
```

#### 2. Chocolatey not found (Windows)
**Symptom**: `Chocolatey is required but not installed`
**Solution**: Install Chocolatey first
```powershell
# Install Chocolatey (run as Administrator)
Set-ExecutionPolicy Bypass -Scope Process -Force
iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
```

#### 3. Permission denied (Linux)
**Symptom**: Installation fails with permission errors
**Solution**: Ensure user has sudo access and packages can be installed

#### 4. Version mismatch warnings
**Symptom**: Tool installed but version doesn't match expected
**Solution**: Usually safe to ignore unless specific version required

### Advanced Testing

#### Test with Clean Environment Simulation
```powershell
# Full test with environment backup/restore
pwsh -File "test-environment.ps1" -SimulateClean

# Review test log
Get-Content "test-environment.log"
```

#### Manual Validation
```powershell
# Check individual tools
node --version
npm --version
firebase --version
gcloud --version
gh --version
```

## Development Notes

### Script Design Principles
- **Fail-fast**: Scripts exit immediately on errors
- **Idempotent**: Safe to run multiple times
- **Cross-platform**: Works on Windows and Linux
- **Comprehensive logging**: Detailed output for troubleshooting
- **Error recovery**: Backup/restore functionality for testing

### Customization
- Modify version requirements in `install-environment.ps1`
- Add additional tools by following existing patterns
- Customize validation criteria in `validate-environment.ps1`

### File Structure
```
scripts/
├── install-environment.ps1    # Main installation script
├── validate-environment.ps1   # Validation and verification
├── test-environment.ps1       # Testing framework
├── path-backup.json          # Generated during testing
├── test-environment.log      # Generated during testing
└── README.md                 # This documentation
```

## Exit Codes

All scripts follow standard exit code conventions:
- **0**: Success
- **1**: Failure or validation errors

This allows for integration with CI/CD pipelines and automation workflows.

## Contributing

When modifying scripts:
1. Test on both Windows and Linux
2. Ensure error handling is comprehensive
3. Update documentation for any new features
4. Test with clean environment simulation
5. Verify idempotent behavior
