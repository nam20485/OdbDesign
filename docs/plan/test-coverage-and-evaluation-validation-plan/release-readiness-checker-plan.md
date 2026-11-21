# Release-Readiness Checker — Design / Plan

**Date:** 2026-11-19 (design); implemented 2026-11-20
**Status:** IMPLEMENTED in the local working tree — `scripts/release_readiness.py` + `scripts/release_readiness_criteria.json` are **not yet committed to the repository** (commit pending in the authoring session; until they land, clones do not contain them and this document is the design + build record)
**Companion to:** [`test-coverage-and-evaluation-validation-plan.md`](test-coverage-and-evaluation-validation-plan.md)
**Deliverable:** an always-runnable utility that (a) reports completed-vs-remaining release-readiness criteria at any time, and (b) autonomously validates that readiness has been achieved by checking every required criterion against real evidence.

---

## 0. Implementation status (as built, 2026-11-20)

Built complete — no lite version, in the authoring working tree (see Status above — the files are not yet in any branch). Files:

- `scripts/release_readiness.py` — engine: registry loader, status model, 8 evaluator
  types (`repo_file_registered`, `repo_file_exists`, `proto_drift`, `gh_workflow_status`,
  `coverage`, `junit`, `evidence`, `probe`), real runtime probes, dependency blocking,
  candidate binding, engine negative controls, `report`/`validate`, human + `--json` output.
- `scripts/release_readiness_criteria.json` — 52 criteria across all 19 scenario groups
  and milestones M0–M6 / C0–C4. One criterion is production-only, so `eval` requires
  51 of the 52 + 4 engine negative controls = **55**; `production` requires all 52 + 4 = 56.
- `scripts/README.md` — usage entry. `release-readiness/` evidence dir is gitignored.

Verified behavior:

- `report` exits 0 with a truthful picture; `validate --profile eval|production` exits 1
  (NOT READY) on the current state. `--no-probes` gives a fast, side-effect-free glance.
- Real findings already surfaced: `M1-01` batch tests not registered; `PKG-01` commercial
  packaging latest run cancelled; `PROTO-01/02` drift in 3 model protos vs both clients.
- Probes attempt real execution: `SEC-01` (F004) launches the server and reports `blocked`
  with the genuine `libprotobuf.so.33.4.0` missing-lib error in this checkout (the dynamic
  runtime triplet is absent here); commercial license/loopback probes report `blocked`
  because the commercial binary is not built. No fabricated passes; clean process teardown.
- Negative controls 4/4 pass (skip/fail/unknown/mixed-candidate all correctly fail the verdict).

How criteria turn green: deliverable/drift/CI criteria evaluate now; coverage criteria need
the M0 tracefile in `release-readiness/evidence/coverage.info`; acceptance criteria need their
run to emit `release-readiness/evidence/<ID>.json` with `{candidate, scenarios:{<ID>:{status:"pass"}}}`;
runtime probes need their subject built (and, for license/loopback, a signed test license from C1).

---

## 1. Why this exists

The validation plan defines the criteria (milestones M0–M6 / C0–C4, scenario groups `SRC-*`…`EVID-*`, numeric thresholds, evidence rules) but has **no executable way to answer "where are we?" or "are we done?"**. Today that answer lives in a human's head and a Markdown checklist. This utility makes it mechanical, repeatable, and auditable.

Two distinct uses, one tool:

| Mode | Question it answers | Behavior |
|---|---|---|
| `report` (default) | "What's done, what's left, what's blocking?" | Evaluates everything it can *right now*, tolerantly. Never fails the run just because work is unfinished. Produces a progress breakdown. |
| `validate` | "Has readiness been achieved for this exact candidate?" | Strict gate. Every required criterion must be `pass` with real, candidate-matched evidence. Anything else → nonzero exit. Suitable for autonomous/CI use. |

**Core principle (per the repo's no-grep-validation directive):** a criterion is satisfied only by *real evidence* — a coverage tracefile + JUnit result, a process exit code, a socket probe, an HTTP/gRPC round trip, a checksum/signature verification, or a structured scenario result. String presence in source/config/logs is never proof. Drift detectors (proto diff, route check) are explicitly labeled as *drift detectors*, not behavior proof.

---

## 2. Honest evaluability split (the key design constraint)

Criteria fall into three kinds, and the tool must be truthful about which it can evaluate today:

| Kind | Examples | Evaluatable now? |
|---|---|---|
| **Deliverable** (repo/CI state) | Required test file exists & registered in CMake; coverage ≥ threshold from latest CI artifact; commercial packaging run succeeded; protos reconciled; skips present in latest results | **Yes** — from repo + CI artifacts/API |
| **Runtime probe** (cheap real check) | F004 fail-closed startup; license rejection exits before bind; loopback socket inventory; a real HTTP/gRPC health call | **Yes, if built** — the tool can execute these directly |
| **Acceptance evidence** (heavy run output) | Exact-package clean-VM rehearsal; customer-corpus workflow; .NET stream equivalence; restart/persistence; SDK consumer build | **Only after the run produces evidence** — until then reported as `pending`, never silently passed |

**Consequence:** on day one most acceptance criteria will report `pending`/`blocked`. That is *correct and useful* — it shows the true remaining work rather than a false green. As M0–M6 / C0–C4 land and acceptance runs emit evidence, the checker lights up.

**No cheap version.** The checker — including every evaluator type and the real runtime probes — is built complete now. What is "later" is the *subject work being measured* (the tests, the packaging fix, the acceptance rehearsals = the main plan's M/C milestones), not the checker. A probe whose subject doesn't exist yet (e.g. the commercial binary won't build) runs, hits the real failure, and reports `blocked`; it reports pass/fail automatically once the work lands. The checker cannot fabricate a `pass` for behavior that hasn't been built and exercised — that honesty is the feature, not a limitation to be patched later.

---

## 3. Components

```
scripts/
  release_readiness.py            # the CLI + evaluator engine (stdlib only)
  release_readiness_criteria.json # the criteria registry (single source of truth)
  check_coverage.py               # proposed coverage gate; feeds SRC-* criteria
release-readiness/                # evidence + reports (gitignored working area)
  evidence/                       # structured results ingested by the checker
  reports/                        # generated progress/verdict reports
```

- **Registry** (`release_readiness_criteria.json`): every criterion from the plan, machine-readable. This is the single source of truth; the plan doc references it so the two don't drift.
- **Engine** (`release_readiness.py`): loads the registry, runs each criterion's evaluator, classifies status, aggregates, emits reports.
- **Evidence dir**: where acceptance runs drop structured results (JUnit/TRX/JSON, LCOV, scenario results, checksums). The checker reads these; it does not fabricate them.

### 3.1 Criterion record

```json
{
  "id": "LIC-003",
  "group": "LIC",
  "milestone": "C2",
  "title": "Expired license exits FailedInit before either socket binds",
  "kind": "runtime",
  "required_for": ["eval"],
  "evaluator": "license_reject_before_bind",
  "params": { "license_case": "expired", "expected_exit": 1 },
  "depends_on": ["C0"],
  "evidence": "release-readiness/evidence/lic-003.json"
}
```

Fields:
- `kind`: `deliverable` | `runtime` | `evidence`.
- `required_for`: `eval` and/or `production` (mirrors the plan's eval-vs-production split). `validate --profile eval` only requires `eval` criteria; `--profile production` requires both.
- `evaluator`: a named engine function, or a `command` spec (run a real script, parse its structured output/exit code).
- `depends_on`: milestone/criterion prerequisites → drives `blocked` status.
- `evidence`: path to the structured result this criterion consumes (for `evidence` kind).

### 3.2 Status model

| Status | Meaning |
|---|---|
| `pass` | Real evidence satisfies the criterion for this candidate |
| `fail` | Real evidence contradicts it (a genuine finding) |
| `pending` | Evaluator/run/artifact not yet produced; work remaining |
| `blocked` | A `depends_on` prerequisite is not `pass` |
| `not-applicable` | Out of scope for this candidate/profile (pre-approved, e.g. no K8s in eval) |
| `unknown` | Evaluator errored / evidence unreadable (treated as **not pass**) |

`validate` passes **only** when every required criterion is `pass` (or pre-approved `not-applicable`). `pending`, `blocked`, `fail`, `unknown` all fail the gate. This is what makes autonomous validation trustworthy.

---

## 4. CLI

```bash
# Progress report — always runnable, tolerant, human + machine output
python3 scripts/release_readiness.py report
python3 scripts/release_readiness.py report --json --candidate <sha-or-digest>

# Strict autonomous gate for a specific candidate + profile.
# Executes the real checks needed to reach a verdict (probes, coverage, drift,
# packaging status) and ingests acceptance evidence; fails on any unmet criterion.
python3 scripts/release_readiness.py validate --profile eval \
    --candidate <sha-or-digest> --evidence-dir release-readiness/evidence
```

- `--candidate`: binds results to a commit/image digest. `validate` rejects evidence produced for a different candidate (prevents "CI passed something slightly different").
- `--profile eval|production`: selects the required criterion set. **Default: `eval`.**
- `--evidence-dir`: where acceptance results live.
- `--json`: machine-readable output for CI/aggregation.
- `--no-probes`: skip executing live runtime probes and evaluate only from existing evidence (for a fast, side-effect-free progress glance). By default the tool **does** execute the real probes it owns — starting servers, asserting exit codes, inspecting sockets, making HTTP/gRPC calls. This is real validation, not a lite mode.
- Exit codes follow repo convention: `0` = all required pass (validate) / report generated (report); `1` = unmet required criteria or findings; `2` = misconfiguration (missing registry/evidence/tool).

### 4.1 Report output (sketch)

```
Release readiness — candidate 8af35d5 (profile: eval)
Overall: 14/55 required criteria pass (25%)   [report mode: informational]

By milestone:
  M0 measurement      ██████████ 5/5   pass
  M1 server linkage   ██████░░░░ 3/5   pending: GrpcBatchStream re-enable, ...
  M2 REST/gRPC        ██░░░░░░░░ 2/11  pending: real HTTP harness, feature streams
  C0 commercial CI    ░░░░░░░░░░ 0/4   blocked: packaging run failed (run 44007546488)
  ...

Blocking the eval gate (first 5):
  - PKG-001  package workflow must succeed        [fail: all 5 runs failed]
  - SEC-001  F004 fail-closed auth                [pending: no test/run yet]
  - LIC-*    license/socket matrix                [blocked by C0]
  ...

Remaining work: 49 criteria (31 pending, 12 blocked, 6 fail)
```

Plus a per-criterion table and a JSON document with every status, evidence path, and candidate binding.

---

## 5. Evaluator framework

A small set of evaluator types, all evidence-driven:

| Type | How it proves the criterion |
|---|---|
| `artifact` | Parse a real file: LCOV tracefile (raw counts → thresholds), JUnit/TRX XML (expected vs passed/skipped), scenario JSON, checksum sidecar |
| `command` | Run a real script/binary, capture exit code + structured output (e.g. `check_coverage.py`, `check-openapi-routes.py` as a *drift* signal, `gh run view` for packaging status) |
| `probe` | Execute a real runtime check: start server subprocess, assert exit code + socket inventory + HTTP/gRPC response (license/loopback/F004/health) |
| `evidence` | Consume a structured result an acceptance run emitted; verify it matches the candidate and the expected scenario ID/count |
| `repo` | Check a deliverable exists & is wired (test file present AND registered in CMake AND appears in a test-results manifest — not just file existence) |

**Negative controls are first-class:** the engine includes self-checks proving that a skipped required test, a missing tool, a changed digest, or a failed RPC makes the verdict fail even if a later health check passes. These run in `validate` and are themselves criteria (`EVID-*`).

---

## 6. What it can report today vs. after infrastructure lands

| Criterion group | Today (no new infra) | After M0–M6 / C0–C4 |
|---|---|---|
| `SRC-*` coverage | Latest CI artifact numbers (73.16% headline; handwritten recalc) — but **no branch data, no complete denominator** → reported as partial/pending until M0 | Full >90% line/func + ≥85% branch gate from `check_coverage.py` |
| `SRC-*` test deliverables | Which required test files exist/registered; skip count from latest results | Real-transport suites present & passing |
| `PKG-*` | Commercial packaging run status via `gh` (currently **fail**) | Passing run + verified artifact |
| `PROTO-*` | Drift detector: canonical vs vendored proto diff (currently **3 files differ**) — labeled drift, not proof | Reconciled + real .NET round trips |
| `SEC-*`, `LIC-*`, `LOOP-*` | Probes built; report `blocked` where the subject binary/behavior doesn't exist yet (e.g. commercial build fails) | Probes execute real checks and report pass/fail |
| `REST/GRPC/DOTNET/E2E/CORP/PERF/...` | `pending` (acceptance runs not built) | Ingested structured evidence |

This table is the point: the tool gives a **truthful progress picture immediately** and becomes the **autonomous gate** as the plan executes.

---

## 7. Integration

- **Plan doc:** add a short section referencing the registry + CLI as the executable companion; the registry encodes the plan's criteria.
- **Coverage gate:** `check_coverage.py` (from the main plan's M0) becomes the `SRC-*` coverage evaluator.
- **CI (optional, later):** a `release-readiness` job runs `report` on PRs (informational) and `validate --profile eval` on a release candidate (blocking). Not wired until the underlying gates exist, to avoid a permanently-red job.
- **Acceptance runs:** each M/C milestone's acceptance harness emits structured results into `release-readiness/evidence/` keyed by scenario ID + candidate, which the checker ingests.

---

## 8. Scope boundaries / non-goals

- The checker is built **complete** — all evaluator types and real runtime probes included. It is not a phased "lite now, real later" tool.
- It **evaluates and aggregates**; it does not itself implement the parser/REST/license tests or fix the packaging pipeline. Those are the main plan's M/C work — the *subject* the checker measures. The checker reports them `pending`/`blocked` until they exist, then measures them for real.
- It does not, by default, *orchestrate* the heavy acceptance runs (clean-VM rehearsal, customer-corpus upload, 24h soak). Building those harnesses is the plan's C3/E2E work — a large effort distinct from checking. Once they exist and emit evidence, the checker ingests it; a future `--orchestrate` could drive them, but that is not part of this tool's initial scope.
- No grep-style pass conditions. No silent pass on missing evidence. No mixing candidates. No false green.

---

## 9. Proposed implementation steps (after approval)

1. Create `release_readiness_criteria.json` encoding all plan criteria (groups, milestones, kinds, required_for, evaluators, dependencies).
2. Implement `release_readiness.py`: registry loader, status engine, **all** evaluator types (`artifact`, `command`, `repo`, `evidence`, `probe`), aggregation, `report`/`validate`, JSON + human output, candidate binding, negative controls. Probes are real and built now; where their subject doesn't exist yet they run, hit the real failure, and report `blocked`.
3. Wire the evaluators that produce real signals today (CI coverage artifact, packaging run status, proto drift, test-file registration, skip count) and the runtime probes (F004, license/loopback, health).
4. Add a plan-doc section + `scripts/README.md` entry documenting usage.
5. Self-test: run `report` (expect mostly `pending`/`blocked` plus a few real signals) and `validate` (expect nonzero, correctly listing blockers); verify negative controls fail the verdict as designed.

**Estimate:** ~2–3 person-days for the complete engine + registry + all evaluators/probes. The *criteria themselves* turn green as the main plan's M/C work lands — that work is estimated separately in the validation plan, not here.

---

## 10. Decisions (locked 2026-11-19)

1. **Autonomy depth — resolved:** no cheap/lite version. The checker is built complete with real probes that execute checks; `validate` does real validation. The only deferred effort is *orchestrating* heavy acceptance runs (clean VMs, customer corpus, soak), which is the main plan's C3/E2E harness work, not the checker. `--no-probes` exists only as an optional fast, side-effect-free progress glance.
2. **Naming — confirmed:** `scripts/release_readiness.py` + `scripts/release_readiness_criteria.json`.
3. **Default profile — confirmed:** `eval`.
