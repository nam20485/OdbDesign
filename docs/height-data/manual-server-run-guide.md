# Manual Server Run Guide - Component Height Investigation

**Date**: January 2025  
**Purpose**: Run OdbDesignServer manually to collect ComponentHeightTracer logs

---

## Quick Start

### 1. Build Configuration

**Any build type works**: ComponentHeightTracer works in both Debug and Release builds. No special build configuration needed.

**Common build paths**:
- Debug: `out\build\x64-debug\Debug\OdbDesignServer.exe`
- Release: `out\build\x64-release\Release\OdbDesignServer.exe`

### 2. Ensure config.json is in Executable Directory

The server looks for `config.json` in this order:
1. `GRPC_CONFIG_PATH` environment variable (if set)
2. Executable directory (where `OdbDesignServer.exe` is located)
3. Current working directory

**Default location**: `OdbDesignServer/config.json` should be copied to the executable directory during build.

**Verify**: Check that `config.json` exists next to `OdbDesignServer.exe`:
```powershell
# Debug build example
Test-Path "out\build\x64-debug\Debug\config.json"
# Release build example
Test-Path "out\build\x64-release\Release\config.json"
```

### 3. Run Server

**Basic command** (Debug build):
```powershell
.\out\build\x64-debug\Debug\OdbDesignServer.exe
```

**Release build**:
```powershell
.\out\build\x64-release\Release\OdbDesignServer.exe
```

**With custom options**:
```powershell
.\out\build\x64-debug\Debug\OdbDesignServer.exe --port 8888 --grpc-port 50051 --designs-dir designs
```

**Common options**:
- `--port <port>` - REST server port (default: 8888)
- `--grpc-port <port>` - gRPC server port (default: 50051)
- `--designs-dir <dir>` - Directory containing design files (default: "designs")
- `--load-design <name>` - Pre-load a specific design on startup
- `--load-all` - Pre-load all designs on startup
- `--disable-authentication` - Disable authentication (useful for testing)
- `--help` - Show usage information

### 4. ComponentHeightTracer Configuration

**No special configuration needed!** The tracer is already configured to trace:
- **designodb_rigidflex**: R70, R56, N5
- **sample_design**: MTG1, TP5, J11
- First 20 failures (components missing height data)

All logging happens automatically - no command-line flags needed.

### 5. Collect Logs

**Server output**: ComponentHeightTracer logs appear in stdout/stderr with prefix `[CompHeightTrace]`

**Filter logs** (if redirecting to file):
```powershell
# Run server and redirect output to file
.\out\build\x64-debug\Debug\OdbDesignServer.exe > server.log 2>&1

# Filter for component height traces
Select-String -Path "server.log" -Pattern "\[CompHeightTrace\]"
```

**Or filter in real-time**:
```powershell
.\out\build\x64-debug\Debug\OdbDesignServer.exe | Select-String -Pattern "\[CompHeightTrace\]"
```

---

## Expected Server Output

### Startup Messages

```
Using default gRPC config (config.json not found)
# OR
Loaded gRPC config from: <path>\config.json

gRPC max message sizes: receive=150MB, send=150MB
gRPC server listening on 0.0.0.0:50051
```

### ComponentHeightTracer Logs

**When designs are loaded/requested**, you'll see logs like:

```
[CompHeightTrace][PARSE_START] Component=R70, Package=48, attrIdString=";0=0.010236"
[CompHeightTrace][PARSE_RESULT] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, .comp_height@index0=YES, key"0"_exists=YES, height_value="0.010236"
[CompHeightTrace][SERIALIZE] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, height_value="0.010236"
[CompHeightTrace][GRPC_RESPONSE] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, height_value="0.010236"
```

---

## Troubleshooting

### config.json Not Found

**Symptom**: Server prints "Using default gRPC config (config.json not found)"

**Solution**: 
1. Check that `config.json` exists in executable directory
2. Or set `GRPC_CONFIG_PATH` environment variable:
   ```powershell
   $env:GRPC_CONFIG_PATH = "D:\src\github\nam20485\OdbDsn\OdbDesignServer\config.json"
   .\out\build\x64-debug\Debug\OdbDesignServer.exe
   ```

### No ComponentHeightTracer Logs

**Possible causes**:
1. Designs not loaded yet - make a gRPC request to load designs
2. Test components not in requested design - verify design contains R70, R56, N5, MTG1, TP5, or J11
3. Logs filtered out - check stdout/stderr capture

**Solution**: Make a gRPC `GetDesign` request for `designodb_rigidflex` or `sample_design`

### Designs Not Found

**Symptom**: gRPC requests return "Design not found"

**Solution**: Ensure designs directory contains:
- `designs/designodb_rigidflex/` (or `.tgz` file)
- `designs/sample_design/` (or `.tgz` file)

Or specify custom directory:
```powershell
.\out\build\x64-debug\Debug\OdbDesignServer.exe --designs-dir "D:\path\to\designs"
```

---

## Example: Full Run with Log Capture

```powershell
# Navigate to project root
cd D:\src\github\nam20485\OdbDsn

# Run server and capture logs
.\out\build\x64-debug\Debug\OdbDesignServer.exe --designs-dir designs > server.log 2>&1

# In another terminal, make gRPC requests (or use your client)
# Then filter logs:
Select-String -Path "server.log" -Pattern "\[CompHeightTrace\]" | Out-File height-traces.log

# View traces
Get-Content height-traces.log
```

---

## Next Steps After Running

1. **Collect logs** with `[CompHeightTrace]` prefix
2. **Analyze logs** using the patterns in `docs/height-data/investigation-guide.md`
3. **Identify root cause** based on where data is lost:
   - Parse time: `[PARSE_RESULT]` shows missing key
   - Serialization: `[SERIALIZE]` shows data but `[GRPC_RESPONSE]` doesn't
   - Transmission: All logs show data but client doesn't receive it

---

## References

- Investigation Guide: `docs/height-data/investigation-guide.md`
- Ready Summary: `docs/height-data/investigation-ready-summary.md`
- Config File: `OdbDesignServer/config.json`
