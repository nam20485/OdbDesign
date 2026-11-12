# Completed Work (Archive)

Historical plans, investigations, and analyses for work that has **shipped or
concluded**. Kept for provenance; nothing here has open items. Active plans:
[`docs/plan/`](../plan/README.md). Excluded from the published GitHub Pages site.

| Area | Docs | Outcome |
|---|---|---|
| gRPC server migration | `grpc/` (plans v1.0→v3.2, architecture guide, impl spec, risks, WBS) | Shipped 2025 — gRPC services in `OdbDesignServer/Services/` (#561); UTF-8 sanitizer wired in (#565) |
| StandardFonts gRPC endpoint | `standardfonts/` | Shipped 2026-09 — `GetStandardFonts` RPC in `service.proto` (client-authored spec + implementation handoffs); tests in `OdbDesignTests/GetStandardFontsTests.cpp` |
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
| Component identity & connectivity epic | `component-connectivity/` (design + plans, handoff, follow-ups, status snapshots, f4-f5 execution plan) | **Complete 2026-10-29** — server Phases 0–2 (#594, #595, #601, #603, #606) + M4.4 (#608); both clients' F1–F6 done; `Connectivity` contract live end-to-end. Residue is decision-based only (M4.1 conditional, M4.2 frozen with REST) |
| PR #609 review cycle | `609-review-fixes-plan.md`, `zcode-workflow-609-review-report.md`, `zcode-workflow-609-findings-adjudication.md` | Three-lens review + adjudication + fix plan executed on PR #609; #609 merged 2026-11-01, deployed + live-accepted 2026-11-02 |
| PR #588 review resolution | `pr-588-review-resolution.md` | All 14 review issues addressed (2026-09-12); W4 recorded as decision D10 in `plan/argocd-deployment-plan.md` |
| Turbot corrupt-protobuf fix | `turbot-getdesign-corrupt-protobuf-fix.md` | Root cause (uninitialized POD member) fixed as #597; server redeployed + Turbot re-verified |
| Server-team hand-off | `server-README.md` | 2026-09-10 client-team handoff; its BLOCKER (Turbot) fixed, done items shipped; still-open items (auth, WS events, gRPC perf) tracked in `plan/server-issues*.md` + `plan/opt/` |
| Program status snapshots | `program-status-2026-10-21.md` (incl. 2026-10-29 completion update), `program-status-2026-11-01.md` | Superseded by [`plan/program-status-2026-11-02.md`](../plan/program-status-2026-11-02.md) |

Note: docs in this archive may reference debug `.txt` dumps and superseded
files that were deleted in the 2026-09-11 cleanup; quoted excerpts live in the
docs themselves.
