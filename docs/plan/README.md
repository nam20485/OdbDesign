# Active Plans

Everything in `docs/plan/` is **live**: either work still to implement or the
decision records that govern it. Finished work lives in
[`docs/completed/`](../completed/README.md); durable reference material lives in
[`docs/reference/`](../reference/README.md).

## In-flight / next up

### Server issues — perf, auth, events
* [`server-issues.md`](server-issues.md) — options & decisions catalog (SI1–SI8, SI19), **all decisions resolved 2026-09-10**.
* [`server-issues-implementation-plan.md`](server-issues-implementation-plan.md) — milestone tracker.
  * **Shipped:** M0.1–M0.5, M1.1 (single-flight cache, #583), M1.2 (RequestLoadDesign, #585), M1.3 (ETag/Cache-Control, #586), M1.4 (serialized-response cache, #587), M3.1 (TLS, #582).
  * **Remaining:** M1.5 designs-dir watcher (optional) · **M2.1–M2.5 auth track** (stopgap auth → Keycloak → JWT middleware + gRPC interceptor → ownership/device-flow → remove Basic) · **M3.2** WebSocket `/events`.

### gRPC phase-2 optimizations (SI19)
* [`opt/phase2-grpc-optimizations.md`](opt/phase2-grpc-optimizations.md) — binding spec. Items 2.1 (proto arenas) and 2.2 (compression-level config) shipped; **item 1.3 (arena allocation in streaming RPCs) is open** — descoped from M1.4 (#587).

### Component identity & server-owned connectivity
* [`component-connectivity.md`](component-connectivity.md) — verified findings, spec evidence, the `Connectivity` contract, and decisions **D1–D6 (open)**.
* [`component-connectivity-implementation-plan.md`](component-connectivity-implementation-plan.md) — milestone tracker. **Nothing started.** Phase 0/1 unblocked; **Phase 2 is gated on `dev/getdesign-include-lists-flag` merging** and D4; Phase 3 spans both client repos.
* [`component-id-issue.md`](component-id-issue.md) — originating client report. Its symptom diagnosis is correct; its conclusion ("no design has real component IDs") is disproved in §4 of the design doc.

### Argo CD GitOps migration
* [`argocd-gitops-handoff.md`](argocd-gitops-handoff.md) — platform handoff: cluster/Argo CD facts, migration approach, secrets policy.
* [`argocd-deployment-plan.md`](argocd-deployment-plan.md) — OdbDesign-side execution plan (Applications, `bump-manifest` CI job, CI-gating debt). **Plan merged (#568); execution not started.**

### IPC-2581 import (future feature, not started)
* [`ipc2581/IPC-2581 Implementation Plan for OdbDesign (Server).md`](ipc2581/IPC-2581%20Implementation%20Plan%20for%20OdbDesign%20%28Server%29.md)
* [`ipc2581/spec-issues.md`](ipc2581/spec-issues.md) — binding maintainer constraints for the implementation (resolve conflicts in favor of this over the plan).
* [`ipc2581/Client Migration Guide_ ODB++ to Unified API.md`](ipc2581/Client%20Migration%20Guide_%20ODB%2B%2B%20to%20Unified%20API.md) — client-facing end-state guide; **describes an API that does not exist yet**.

## Decision records (decided + shipped, kept for cited sections)
* [`https-tls-options.md`](https-tls-options.md) — REST TLS = Option C (cert-manager private CA, IP SANs); shipped as M3.1 (#582). Kept here because `deploy/kube/cert-manager/*`, `scripts/make-tls-ca.sh`, and the server-issues docs cite its sections.
