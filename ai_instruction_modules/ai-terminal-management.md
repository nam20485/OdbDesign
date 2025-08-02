# Terminal Management Best Practices

## Core Principle: Reuse Terminal Sessions

### Default Behavior (Recommended)
```
run_in_terminal with isBackground=false (or omitted)
```
- **Use for**: Sequential commands, builds, tests, file operations
- **Behavior**: Reuses the same terminal session
- **Benefits**: Efficient, maintains context, cleaner process list

### Background Process Behavior (Use Sparingly)
```
run_in_terminal with isBackground=true
```
- **Use for**: Long-running services, servers, watch tasks, Docker containers
- **Behavior**: Creates new terminal session that runs independently
- **When to use**: Only when you need the process to continue running while executing other commands
- **⚠️ IMPORTANT**: Always stop any existing background process BEFORE starting a new one to avoid port conflicts and resource waste

## Examples

### ✅ Correct Usage
```
# Sequential commands (reuse terminal)
run_in_terminal: dotnet build (isBackground=false)
run_in_terminal: dotnet test (isBackground=false)
run_in_terminal: git status (isBackground=false)

# Background service (new terminal)
run_in_terminal: dotnet run --project WebApp (isBackground=true)
```

### ❌ Incorrect Usage
```
# Don't create new terminals for sequential commands
run_in_terminal: dotnet build (isBackground=true)  # WRONG
run_in_terminal: dotnet test (isBackground=true)   # WRONG
```

## System Impact
- **Multiple terminals waste system resources**
- **Clutters process list and VS Code terminal tabs**
- **Makes debugging and monitoring more difficult**
- **Can lead to orphaned processes**

## Cleanup Strategy
- Monitor running PowerShell/terminal processes
- Clean up unnecessary background processes
- Use `Get-Process` to check for resource usage

---
*This practice ensures efficient terminal usage and better system resource management.*
