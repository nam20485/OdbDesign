# REST API connectivity verification + tracing (server-side)

Date: 2026-01-12  
Repo/branch: `nam20485/OdbDesign` – `feature-grpc/debug-output-issues`

## Summary

Client-side config looks correct: the client is attempting to reach `http://192.168.1.187:8888/designs`.

The observed symptom is a **TCP connection timeout** from the client machine (`192.168.1.30`) to the server machine (`192.168.1.187`) on port **8888**.

That almost always points to one of:
- REST server not listening on a non-loopback interface (bind = `127.0.0.1` / `localhost`)
- Host firewall blocking inbound TCP/8888
- Docker publish address is loopback-only (e.g., `127.0.0.1:8888:8888`) or host networking rules block it
- Network/routing/VLAN isolation (less likely on same /24, but possible)

This repo’s `compose.yml` publishes `8888:8888` (no explicit loopback binding), which *should* expose the port to the LAN **if** host firewall and Docker networking allow it.

## What we changed (to help diagnose)

We added two server-side capabilities:

1) **Explicit REST bind address**
- New CLI arg: `--bind <ip|host>`
- Default: `0.0.0.0` (listen on all interfaces)

2) **Optional per-request HTTP tracing**
- New CLI arg: `--http-trace`
- When enabled, the server logs a short line on request start + finish:
  - method
  - URL
  - Host header
  - best-effort remote address (Crow-version dependent)
  - response status
  - request duration (ms)

These changes remove ambiguity about what interface the REST server is listening on, and give us better logs once connectivity is restored.

## Files changed

- `OdbDesignLib/App/OdbDesignArgs.h`
- `OdbDesignLib/App/OdbDesignArgs.cpp`
- `Utils/HttpTraceMiddleware.h` (new)
- `Utils/crow_win.h`
- `OdbDesignServer/OdbServerAppBase.cpp`

## How to run with tracing

### Native (running the executable)
Run the server with explicit bind and tracing enabled:
- `--bind 0.0.0.0`
- `--port 8888`
- `--http-trace`

Then hit a health endpoint from another machine:
- `GET http://<server-ip>:8888/healthz`
- `GET http://<server-ip>:8888/ready`

### Docker (compose)
`compose.yml` uses:
- `ports: - 8888:8888`

Confirm the *actual* publish binding on the server host:
- Linux: `docker ps` should show `0.0.0.0:8888->8888/tcp` (not `127.0.0.1:8888->8888/tcp`)
- Windows: `docker ps` similarly lists the publish binding

If it shows loopback-only binding, the LAN will not be able to connect.

## Verification checklist (server machine 192.168.1.187)

### 1) Is something listening on 8888 on all interfaces?
Windows:
- `netstat -an | Select-String ":8888"`

Expected for LAN access:
- `TCP    0.0.0.0:8888    0.0.0.0:0    LISTENING`

If you only see:
- `TCP    127.0.0.1:8888  0.0.0.0:0    LISTENING`

…then the server is bound to loopback only.

### 2) Is Windows Firewall blocking it?
On the server:
- check inbound rule(s) permitting TCP/8888
- create an allow rule (scoped to the subnet if desired)

### 3) Can the server be reached from the client host?
On the client machine (192.168.1.30):
- `Test-NetConnection -ComputerName 192.168.1.187 -Port 8888`

Interpretation:
- `TcpTestSucceeded: False` + timeout => typically firewall or bind/publish issue
- `TcpTestSucceeded: True` but HTTP fails => app auth/path issue (not network)

## Notes for the client team

- Your logs already prove the client is targeting `192.168.1.187:8888` and setting `HttpClient.BaseAddress` correctly.
- Because the failure is a TCP timeout (10060), application-layer fixes (auth headers, base URL formatting, etc.) won’t help until connectivity is restored.

## Next data to collect

To close this out quickly, please capture and share:

1) On **server** (192.168.1.187):
- `netstat -an | Select-String ":8888"`
- firewall rule evidence for TCP/8888 inbound allow
- (if Docker) `docker ps` showing the published ports for the container

2) On **client** (192.168.1.30):
- `Test-NetConnection -ComputerName 192.168.1.187 -Port 8888`

Once TCP connects, enabling `--http-trace` will give request-level evidence (auth failures, unexpected routes, etc.).
