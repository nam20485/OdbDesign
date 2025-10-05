# CodeQL Security Analysis Configuration

This directory contains the CodeQL security analysis configuration for the OdbDesign C++ project.

## Files Overview

### `.github/codeql-config.yml`
Main CodeQL configuration file that:
- Defines analysis paths (focuses on source code, excludes dependencies)
- Configures comprehensive security query suites
- Filters out false positives and noise
- Includes custom security queries specific to this project

### `.github/workflows/codeql.yml`
GitHub Actions workflow that:
- Runs CodeQL analysis on code changes and weekly schedule
- Supports both Linux and Windows analysis (Windows commented out by default)
- Uses optimized build processes with vcpkg dependency management
- Implements trap caching for faster analysis
- Uploads detailed security findings to GitHub Security tab

### `.github/codeql/cpp-queries/`
Custom CodeQL queries directory containing:
- `odbdesign-security.ql`: Custom security patterns specific to this C++ codebase
- `odbdesign-suite.qls`: Query suite combining custom and standard security queries

## Analysis Scope

The CodeQL analysis covers:
- **OdbDesignServer/**: Web server implementation
- **OdbDesignApp/**: Desktop application
- **OdbDesignLib/**: Core library functionality  
- **Utils/**: Utility functions and classes

Excluded from analysis:
- Generated protobuf code (`*.pb.h`, `*.pb.cc`)
- Third-party dependencies (`vcpkg/`, `vcpkg_installed/`)
- Build artifacts (`out/`, `build/`, `CMakeFiles/`)

## Security Checks

The analysis includes checks for:
- **CWE-078**: Command Injection
- **CWE-079**: Cross-Site Scripting  
- **CWE-089**: SQL Injection
- **CWE-120**: Buffer Write vulnerabilities
- **CWE-121**: Stack Buffer Overflow
- **CWE-190**: Integer Overflow
- **CWE-416**: Use After Free
- **CWE-457**: Use of Uninitialized Variables
- **CWE-476**: Null Pointer Dereference
- **Custom**: Unsafe string operations (strcpy, sprintf, etc.)

## Viewing Results

Security findings are available in multiple places:
1. **GitHub Security Tab**: Navigate to Security → Code scanning alerts
2. **Pull Request Comments**: Findings automatically commented on PRs
3. **Workflow Logs**: Detailed analysis logs in Actions tab
4. **SARIF Files**: Machine-readable results uploaded as artifacts

## Customization

To add custom security checks:
1. Create new `.ql` files in `.github/codeql/cpp-queries/`
2. Add them to `odbdesign-suite.qls`
3. Test locally with `codeql database create` and `codeql database analyze`

To modify analysis scope:
1. Update `paths` and `paths-ignore` in `codeql-config.yml`
2. Adjust query filters to reduce false positives

## Performance

Analysis typically takes 4-8 hours for comprehensive security scanning.
- TRAP caching reduces subsequent runs by ~30-50%
- Parallel analysis across multiple cores
- Incremental analysis on pull requests

## Troubleshooting

Common issues and solutions:
- **Build failures**: Check vcpkg dependencies and CMake presets
- **False positives**: Add exclusions to `query-filters` in config
- **Long analysis times**: Consider reducing query suites or analysis scope
- **Missing findings**: Verify paths are included and queries are enabled

For support, see:
- [CodeQL Documentation](https://codeql.github.com/docs/)
- [GitHub Security Documentation](https://docs.github.com/en/code-security/code-scanning)
