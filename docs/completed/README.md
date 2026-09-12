# Completed Work (Archive)

Historical plans, investigations, and analyses for work that has **shipped or
concluded**. Kept for provenance; nothing here has open items. Active plans:
[`docs/plan/`](../plan/README.md). Excluded from the published GitHub Pages site.

| Area | Docs | Outcome |
|---|---|---|
| gRPC server migration | `grpc/` (4 plans + spec, v1→v3.2) | Shipped 2025 — gRPC services in `OdbDesignServer/Services/` (#561); UTF-8 sanitizer wired in (#565) |
| Component-height data investigation | `height-data/` | Concluded Jan 2026 — server correct; default 1.0 mm height is expected behavior; no fix required |
| Client rendering contract + debug cycle | `render-components2/` (incl. `debug-output-issues/`) | Jan 2025 — `GetLayerFeaturesBatchStream` v2 contract implemented; debug-issue fixes (150 MB limits etc.) shipped |
| Render API gaps | `render-gaps2/`, `PR-503-TODO-PLAN.md` | PR #503 merged 2025-12-23; S1/S3 implemented |
| gRPC perf audit + client handoff | `opt/` | Superseded by [`plan/server-issues.md`](../plan/server-issues.md) (SI19) + [`plan/opt/phase2-grpc-optimizations.md`](../plan/opt/phase2-grpc-optimizations.md) |
| CI vcpkg binary cache | `vcpkg-binary-cache/` | Implemented — GH Packages NuGet feed (#501), hardened with `VCPKG_COMMIT` pin + split restore/save (fab778c); live guide: [`reference/vcpkg/localdev-gh-binary-cacheing.md`](../reference/vcpkg/localdev-gh-binary-cacheing.md) |
| k3s VM cluster build | `k3s-vm-cluster-plan.md` (+ `-review.md`, R1–R22) | Cluster built & running (#563); ops facts now in AGENTS.md |
| Argo CD install | `argocd-install-plan.md` | Installed (Argo CD v3.5.2 on the cluster); script never merged, moot. Migration itself: see [`plan/argocd-deployment-plan.md`](../plan/argocd-deployment-plan.md) |
| linux-dynamic-release preset | `linux-dynamic-release-plan.md` | Rolled out as main Linux preset (#564); fixes dual-protobuf SIGABRT |
| GitHub Pages coverage | `gh-pages-coverage-integration.md` | Implemented — per-branch coverage in `jekyll-gh-pages.yml` |
| CI workflow audit | `workflow-optimization-analysis.md` | Nov 2025 point-in-time analysis; recommendations landed |
| CodeQL setup | `CODEQL_SETUP_COMPLETE.md` | Config done; workflow since disabled (`.github/workflows/disabled/codeql.yml`) |

Note: docs in this archive may reference debug `.txt` dumps and superseded
files that were deleted in the 2026-09-11 cleanup; quoted excerpts live in the
docs themselves.
