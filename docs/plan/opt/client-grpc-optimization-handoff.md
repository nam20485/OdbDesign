# Client gRPC Optimization Handoff

**Date**: February 8, 2026  
**From**: Server Team (OdbDesignServer)  
**To**: Client Dev Team (OdbDesign3DClient)  
**Context**: Server-side Phase 1 stability fixes have been **IMPLEMENTED** on `feature/grpc` branch (PR #498)  
**Last Updated**: February 8, 2026 — Phase 1 build verified

---

## Server Changes Completed (for your awareness)

These changes have been implemented on the server. Items marked **ACTION REQUIRED** need client-side updates.

### 1. DesignCache Thread Safety (Internal — no client impact)

> | Field | Details |
> |:------|:--------|
> | **Status** | ✅ **IMPLEMENTED** |
> | **Result** | Added `std::shared_mutex` with double-checked locking on all cache operations. |
> | **Notes** | |
> | **Client Dev Team** | No client changes needed. Improved stability under concurrent load. |

Server added `std::shared_mutex` to `DesignCache` for concurrent access protection. `GetDesign`/`GetFileArchive` use double-checked locking (shared_lock for reads, unique_lock for writes). `LoadDesign`/`LoadFileArchive` restructured as lock-free I/O helpers to prevent deadlocks. **No client changes needed.**

### 2. gRPC Thread Pool Limits (Internal — no client impact)

> | Field | Details |
> |:------|:--------|
> | **Status** | ✅ **IMPLEMENTED** |
> | **Result** | Added `grpc::ResourceQuota` with `max_threads=8`, `min_pollers=1`. Configurable via `config.json`. |
> | **Notes** | |
> | **Client Dev Team** | No client changes needed. Under extreme concurrency, requests queue (by design). Consider adding `RESOURCE_EXHAUSTED` to retry policy. |

Server added `grpc::ResourceQuota` with configurable max threads (default 8) and min pollers (default 1). Under extreme concurrent load, excess requests queue rather than spawning unbounded threads. **No client changes needed**, but be aware that under very heavy concurrency the server may respond slightly slower (by design, to stay stable).

### 3. Server Keepalive Enabled (**ACTION REQUIRED**)

> | Field | Details |
> |:------|:--------|
> | **Status** | ✅ **IMPLEMENTED** (server-side) |
> | **Result** | Keepalive channel args applied. Configurable via `config.json` `keepalive` section. |
> | **Notes** | |
> | **Client Dev Team** | **ACTION REQUIRED** — Add matching client-side keepalive config. See next section below. |

Server keepalive pings are now active:
- **Keepalive time**: 30 seconds (server sends ping every 30s on idle connections)
- **Keepalive timeout**: 10 seconds (connection closed if no response in 10s)
- **Permit without calls**: enabled (pings even when no active RPCs)

---

---

## ACTION REQUIRED: Client Keepalive Configuration

> | Field | Details |
> |:------|:--------|
> | **Status** | ⏳ **CLIENT ACTION PENDING** |
> | **Result** | Server-side keepalive implemented. Client needs matching config. |
> | **Notes** | |
> | **Client Dev Team** | Add `KeepAlivePingDelay`, `KeepAlivePingTimeout`, `KeepAlivePingPolicy` to `SocketsHttpHandler`. |

### Problem
The server now sends keepalive pings every 30 seconds. The .NET gRPC client (`GrpcChannelFactory.cs`) currently has **no keepalive configuration**. While this won't break anything immediately, configuring client-side keepalive provides:
- Faster detection of server restarts/crashes
- Prevention of NAT/firewall timeouts killing idle connections
- Symmetric keepalive for bidirectional health monitoring

### Recommended Change

In `GrpcChannelFactory.cs`, update the channel creation:

```csharp
var handler = new SocketsHttpHandler
{
    // Keepalive: detect dead server connections quickly
    KeepAlivePingDelay = TimeSpan.FromSeconds(30),
    KeepAlivePingTimeout = TimeSpan.FromSeconds(10),
    KeepAlivePingPolicy = HttpKeepAlivePingPolicy.Always,
    
    // Connection pooling (recommended)
    PooledConnectionIdleTimeout = TimeSpan.FromMinutes(5),
    EnableMultipleHttp2Connections = true
};

var channel = GrpcChannel.ForAddress(endpoint, new GrpcChannelOptions
{
    MaxReceiveMessageSize = 150 * 1024 * 1024,  // Updated: match server's 150MB
    MaxSendMessageSize = 150 * 1024 * 1024,      // Updated: match server's 150MB
    CompressionProviders = new List<ICompressionProvider>
    {
        new GzipCompressionProvider(CompressionLevel.Optimal)
    },
    HttpHandler = handler
});
```

### Priority: **MEDIUM**
Not urgent — the server keepalive will work without client changes. But adding it improves resilience.

---

---

## ACTION REQUIRED: Message Size Mismatch

> | Field | Details |
> |:------|:--------|
> | **Status** | ⏳ **CLIENT ACTION PENDING** |
> | **Result** | Server at 150MB, client at 100MB. Mismatch creates risk of `RESOURCE_EXHAUSTED` on large designs. |
> | **Notes** | |
> | **Client Dev Team** | Update `MaxReceiveMessageSize` and `MaxSendMessageSize` to `150 * 1024 * 1024`. |

### Problem
The server is configured with **150MB** message limits, but the client is set to **100MB**:

| Setting | Server | Client | Issue |
|---------|--------|--------|-------|
| MaxReceiveMessageSize | 150 MB | 100 MB | Client will reject responses >100MB |
| MaxSendMessageSize | 150 MB | 100 MB | Client limits outbound to 100MB |

For most features/layers this won't matter, but a very large full-design `GetDesign` response could exceed 100MB.

### Recommended Change

In `GrpcChannelFactory.cs`:

```csharp
MaxReceiveMessageSize = 150 * 1024 * 1024,  // Match server: 150 MB
MaxSendMessageSize = 150 * 1024 * 1024,      // Match server: 150 MB
```

**Alternatively**: Make these configurable via `appsettings.json`:

```json
{
  "GrpcSettings": {
    "MaxReceiveMessageSizeMB": 150,
    "MaxSendMessageSizeMB": 150
  }
}
```

### Priority: **HIGH** — Prevents `RESOURCE_EXHAUSTED` errors on large designs

---

---

## RECOMMENDED: Client-Side Optimizations

These are not required for server Phase 1 compatibility, but would improve client performance.

---

### 1. Batch Streaming: Request-Level Compression

> | Field | Details |
> |:------|:--------|
> | **Status** | 💡 **RECOMMENDED** |
> | **Result** | — |
> | **Notes** | |
> | **Client Dev Team** | Optional. Add per-call `gzip` compression headers for streaming RPCs. |

Currently, the client registers `GzipCompressionProvider` at the channel level. For batch streaming RPC calls specifically, you can also set per-call compression headers:

```csharp
var callOptions = new CallOptions()
    .WithCompression("gzip");

using var call = client.GetLayerFeaturesBatchStream(request, callOptions);
```

This explicitly requests compressed responses for the large streaming calls, even if the server's default compression changes in the future.

---

### 2. Channel Reuse / Connection Pooling

> | Field | Details |
> |:------|:--------|
> | **Status** | 💡 **RECOMMENDED** |
> | **Result** | — |
> | **Notes** | |
> | **Client Dev Team** | Verify `GrpcChannel` is singleton per endpoint. Creating per-request is expensive. |

Ensure `GrpcChannel` is a **singleton** (or one per server endpoint). Creating a new channel per request is expensive:

```csharp
// Good: Singleton channel
services.AddSingleton<GrpcChannel>(sp => GrpcChannelFactory.Create(endpoint));

// Bad: New channel per call
var channel = GrpcChannelFactory.Create(endpoint);  // Don't do this per-request
```

If already a singleton — great, no change needed.

---

### 3. Streaming Buffer / Backpressure

> | Field | Details |
> |:------|:--------|
> | **Status** | 💡 **RECOMMENDED** |
> | **Result** | — |
> | **Notes** | |
> | **Client Dev Team** | Consider bounded channel for backpressure if processing is slower than streaming. |

For `GetLayerFeaturesBatchStream`, if the client is doing heavy processing per batch (e.g., 3D mesh generation), consider:

```csharp
// Process batches with bounded parallelism to avoid memory spikes
var channel = Channel.CreateBounded<FeatureRecordBatch>(new BoundedChannelOptions(4)
{
    FullMode = BoundedChannelFullMode.Wait  // Backpressure to server
});

// Producer: read from gRPC stream
_ = Task.Run(async () =>
{
    await foreach (var batch in call.ResponseStream.ReadAllAsync())
    {
        await channel.Writer.WriteAsync(batch);
    }
    channel.Writer.Complete();
});

// Consumer: process batches
await foreach (var batch in channel.Reader.ReadAllAsync())
{
    ProcessBatch(batch);
}
```

This prevents unbounded memory growth when the server streams faster than the client can process.

---

### 4. Retry Policy: Add `RESOURCE_EXHAUSTED` Backoff

> | Field | Details |
> |:------|:--------|
> | **Status** | 💡 **RECOMMENDED** |
> | **Result** | — |
> | **Notes** | |
> | **Client Dev Team** | Ensure `StatusCode.ResourceExhausted` is in transient error list for Polly retry. |

With server thread pool limits, the server may occasionally return `RESOURCE_EXHAUSTED` under extreme load. Ensure this is in the transient error list (it likely already is based on existing `IsTransientError` code):

```csharp
.Handle<RpcException>(ex =>
    ex.StatusCode == StatusCode.Unavailable ||
    ex.StatusCode == StatusCode.DeadlineExceeded ||
    ex.StatusCode == StatusCode.Aborted ||
    ex.StatusCode == StatusCode.Internal ||
    ex.StatusCode == StatusCode.ResourceExhausted)  // Ensure this is included
```

---

### 5. Deadline Propagation

> | Field | Details |
> |:------|:--------|
> | **Status** | 💡 **RECOMMENDED** |
> | **Result** | — |
> | **Notes** | |
> | **Client Dev Team** | Set explicit deadlines on streaming RPCs (e.g., 5 min) to prevent zombie streams. |

For streaming RPCs, set explicit deadlines to prevent zombie streams:

```csharp
var deadline = DateTime.UtcNow.AddMinutes(5);
var callOptions = new CallOptions(deadline: deadline);

using var call = client.GetLayerFeaturesBatchStream(request, callOptions);
```

This works with the server's new keepalive to ensure both sides clean up stale streams.

---

## Configuration Summary: Aligned Server + Client

### Target Configuration (after both sides update)

| Setting | Server (`config.json`) | Client (`appsettings.json` / code) |
|---------|----------------------|-----------------------------------|
| Max Receive Message | 150 MB | 150 MB |
| Max Send Message | 150 MB | 150 MB |
| Compression | gzip (enabled) | gzip (Optimal) |
| Keepalive Ping Interval | 30 sec | 30 sec |
| Keepalive Timeout | 10 sec | 10 sec |
| Batch Size | 500 (server-configured) | N/A (server decides) |
| Max Threads | 8 (server-configured) | N/A |
| Retry Attempts | N/A | 3 |
| Retry Backoff | N/A | Exponential (1000ms base) |
| Timeout | N/A | 300 sec (5 min) |

---

## Timeline

| Item | Priority | Effort |
|------|----------|--------|
| Message size alignment (100→150 MB) | **HIGH** | 5 min |
| Keepalive configuration | **MEDIUM** | 30 min |
| Channel reuse verification | **LOW** | 15 min |
| Per-call compression headers | **LOW** | 15 min |
| Streaming backpressure | **LOW** | 1-2 hours |

---

## Questions for Client Team

1. Is `GrpcChannel` already a singleton, or is a new one created per operation?
2. Have you observed any `RESOURCE_EXHAUSTED` errors in testing with large designs?
3. Is streaming backpressure relevant to your current architecture, or does the UI handle batches as fast as they arrive?
4. Any interest in `appsettings.json` configuration for message sizes / keepalive, or are hardcoded values acceptable?

---

*This document accompanies the server-side changes in PR #498 on the `feature/grpc` branch.*
