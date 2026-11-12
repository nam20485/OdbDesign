# Active Plans

Everything in `docs/plan/` is **live**: either work still to implement or the
decision records that govern it. Finished work lives in
[`docs/completed/`](https://github.com/nam20485/OdbDesign/blob/development/docs/completed/README.md); durable reference material lives in
[`docs/reference/`](../reference/README.md).

## In-flight / next up

### Server issues — perf, auth, events
* [`server-issues.md`](server-issues.md) — options & decisions catalog (SI1–SI8, SI19), **all decisions resolved 2026-09-10**.
* [`server-issues-implementation-plan.md`](server-issues-implementation-plan.md) — milestone tracker.
  * **Shipped:** M0.1–M0.5, M1.1 (single-flight cache, #583), M1.2 (RequestLoadDesign, #585), M1.3 (ETag/Cache-Control, #586), M1.4 (serialized-response cache, #587), M3.1 (TLS, #582).
  * **Remaining:** M1.5 designs-dir watcher (optional) · **M2.1–M2.5 auth track** (stopgap auth → Keycloak → JWT middleware + gRPC interceptor → ownership/device-flow → remove Basic) · **M3.2** WebSocket `/events`.

### gRPC phase-2 optimizations (SI19)
* [`opt/phase2-grpc-optimizations.md`](opt/phase2-grpc-optimizations.md) — binding spec. Items 2.1 (proto arenas) and 2.2 (compression-level config) shipped; **item 1.3 (arena allocation in streaming RPCs) is open** — descoped from M1.4 (#587).

### GetDesign streaming / OOM fix (M6)
* [`getdesign-streaming-oom-plan.md`](getdesign-streaming-oom-plan.md) — M6.1–M6.3 **shipped** (#609, merged 2026-11-01, deployed + live-accepted): `GetDesignStream` + DesignChunk contract, dispatch guard, disk-spool warm cache. **Open:** M6.4 info-client stream adoption (decision owed — its two big designs render silently empty on a post-#609 server), M6.6 residency budget (queued, branch not cut).
* [`m6-4-client-fanout.md`](m6-4-client-fanout.md) — client fan-out record: 3D client done (#67), info client proto-sync done (#27); the §4 info-client stream deferral premise was wrong and is corrected in-doc.

### Argo CD GitOps migration
* [`argocd-gitops-handoff.md`](argocd-gitops-handoff.md) — platform handoff: cluster/Argo CD facts, migration approach, secrets policy.
* [`argocd-deployment-plan.md`](argocd-deployment-plan.md) — OdbDesign-side execution plan (Applications, `bump-manifest` CI job, CI-gating debt). **Plan merged (#568); execution not started** (zero Application CRs on the cluster).

### Website, API docs & CI reporting (#611–#614)
* [`issues-611-614-website-cdash-plan.md`](issues-611-614-website-cdash-plan.md) — **decided 2026-11-07 (D1–D7), ready for execution**; no repo changes yet. CDash, GH Pages refactor, Doxygen expansion, apex-domain 404.

### Client unification
* [`client-unification-analysis.md`](client-unification-analysis.md) — adopted 2026-11-08: phased shared-library extraction toward `odbdesign-protoc` (Option 2). **Execution not started.**

### Commercialization
* `commercialization/` — *kept as local working copies only (gitignored); not part of the public plan index.*

### Security audit
* `comprehensive-security-audit/` — *kept as local working copies only (gitignored); not part of the public plan index.*

### Backlog / parked
* [`fastBuild-presets.md`](fastBuild-presets.md) — approved plan (tooling + 4 FASTBuild CMake presets), **never executed** — no presets exist in `CMakePresets.json`.

## Program status
* [`program-status-2026-11-02.md`](program-status-2026-11-02.md) — latest cross-repo snapshot (server + 3D + info clients). Predecessors archived under `docs/completed/`.

## Decision records (decided + shipped, kept for cited sections)
* [`https-tls-options.md`](https-tls-options.md) — REST TLS = Option C (cert-manager private CA, IP SANs); shipped as M3.1 (#582). Kept here because `deploy/kube/cert-manager/*`, `scripts/make-tls-ca.sh`, and the server-issues docs cite its sections.
* [`vcpkg-github-packages-outage-fix.md`](vcpkg-github-packages-outage-fix.md) — root cause + validated fix for the GitHub Packages binary-cache outage (both RCs), **shipped as #618 (2026-11-08)**. Kept here because `cmake-multi-platform.yml` and `vcpkg-cache-warm.yml` cite §3.

## IPC-2581 import (future feature, not started)
* [`ipc2581/IPC-2581 Implementation Plan for OdbDesign (Server).md`](ipc2581/IPC-2581%20Implementation%20Plan%20for%20OdbDesign%20%28Server%29.md)
* [`ipc2581/spec-issues.md`](ipc2581/spec-issues.md) — binding maintainer constraints for the implementation (resolve conflicts in favor of this over the plan).
* [`ipc2581/Client Migration Guide_ ODB++ to Unified API.md`](ipc2581/Client%20Migration%20Guide_%20ODB%2B%2B%20to%20Unified%20API.md) — client-facing end-state guide; **describes an API that does not exist yet**.
