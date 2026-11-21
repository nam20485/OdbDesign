# eval-plan.json — Release Manifest Schema + Evidence Aggregator

**Date:** 2026-11-20
**Workstream:** X2 (evidence aggregator + eval-plan.json manifest + candidate binding)
**Companion to:** [`test-coverage-and-evaluation-validation-plan.md`](test-coverage-and-evaluation-validation-plan.md) §10 (acceptance protocol, universal pass rules) and §12 (evidence bundle); [`release-readiness-checker-plan.md`](release-readiness-checker-plan.md)
**Implementation:** `scripts/eval_manifest.py` (stdlib only) + `scripts/eval-plan.example.json` (schema-valid example with placeholder digests)

---

## 1. Purpose

The validation plan §10.1 requires one **immutable, machine-readable release manifest** (`eval-plan.json`) that freezes the identity of a release candidate, and §10.2 requires an **aggregator** whose universal pass rules are enforced mechanically: every required scenario must return a structured pass; skips, missing evidence, candidate mismatches, or failed RPCs fail the overall verdict even when later health checks pass.

`scripts/eval_manifest.py` implements both: schema validation for the manifest, and the aggregation of the structured scenario results the acceptance runs emit into `release-readiness/evidence/`. `scripts/release_readiness.py validate --manifest eval-plan.json` gates readiness on that aggregation on top of the 52 criteria (without `--manifest`, existing behavior is unchanged).

## 2. Manifest schema (`eval-plan.json`)

All fields are validated by `eval_manifest.validate_manifest()`. Unknown extra top-level keys are tolerated (forward compatibility); the fields below are enforced.

| Field | Type | Required | Notes |
|---|---|---|---|
| `manifest_version` | int | yes | Must be the **integer** `1` (strict: `true` and `1.0` are rejected). |
| `release_id` | string | yes | Non-empty release identifier. |
| `created_utc` | string | yes | Manifest freeze timestamp. |
| `candidate` | string | yes | **The canonical candidate binding token.** Every evidence file's `candidate` field must equal this string exactly; anything else is rejected (mixed candidates). |
| `identity` | object | yes | Candidate identity. `upstream_commit` required. Optional: `commercial_commit`, `test_harness_commit`, `image_digest` (e.g. `sha256:…`), `client_commits` (`{client: commit}`), `artifacts` (list of `{name, kind, sha256}`). |
| `protos` | object | recommended | `{proto_path: sha256}` — complete shipped proto graph hashes (plan §10.1). |
| `openapi` | object | recommended | `{source, sha256}` — OpenAPI source hash. |
| `support_matrix` | list | yes | Non-empty; each entry has `platform` (non-empty) plus optional `os`, `runtime`, `hardware`, `notes`. |
| `network_boundary` | object | yes | Non-empty, e.g. `{"rest": "loopback", "grpc": "loopback", "mode": "eval-loopback-only"}`. |
| `corpus` | list | recommended | Entries: `alias`, `sha256`, `size_bytes` (int ≥ 0), `privacy` (non-empty), optional `golden_version`. Aliases + hashes only — never proprietary archive content (§12.5). |
| `resource_envelope` | object | yes | Must contain `memory`, `disk`, `timeouts` (object or scalar); optional `performance` acceptance envelope. |
| `required_scenarios` | object | yes | `all_platforms`: list of scenario IDs required regardless of platform; `by_platform`: `{platform: [scenario IDs]}`. **At least one required ID overall** (platform lists that are all empty do not count — an empty gate is rejected); duplicates within a list rejected; an ID appearing in **more than one bucket** (`all_platforms` + a platform list) rejected — each ID lives in exactly one requirement bucket. |
| `not_applicable_scenarios` | list | optional | Preapproved N/A entries: `{scenario, reason}`, optional `platform`. Platform `not-applicable` is a **preapproved support-matrix entry, never a run-time decision** (§10.2). |
| `signing_keys` | list | yes | Public fingerprint entries. **Only `fingerprint`, `purpose`, `algorithm` fields are allowed**; secret-like field names (`private*`, `secret*`, `password*`, `token*`, `credential*`, …) are rejected outright. Never private keys, passwords, or reusable license documents (§12.5). |

All `sha256` fields must be lowercase 64-hex. A manifest that fails any rule is **misconfiguration → exit 2** (`eval_manifest.py check --manifest …`).

### 2.1 Path safety (fail closed)

Scenario IDs, platform names, evidence file names, and operator paths are all treated as untrusted input to path expressions:

- **Scenario IDs / platform names** (in `required_scenarios`, `by_platform` keys, `not_applicable_scenarios`) must match `^[A-Za-z0-9][A-Za-z0-9._-]*$` and never be `.` or `..` — this bans `/`, `\`, whitespace, and traversal outright. Violations are schema rejections → exit 2, never executed.
- **Evidence file names** must pass the same charset; **evidence scenario IDs** (the `scenarios` keys inside evidence files) too — an unsafe ID is a binding violation (hard FAIL) and its result is discarded.
- **Confinement:** every evidence file is read only if its realpath stays inside the evidence dir's realpath (`os.path.realpath` + `startswith(dir + os.sep)`, the canonical path-confinement barrier) and it is a regular file — a symlink pointing outside the evidence directory is recorded as a binding violation and **never read**.
- **Operator paths:** `--manifest`, `--output`, and `--evidence-dir` must not contain `..` path components (`..` → exit 2 misconfig; absolute paths are allowed). Manifests and evidence directories must additionally realpath into an allowed root tree — the repository, its sibling-repos parent, or the system temp directory; anything else is exit 2. The confinement barrier is an inline `os.path.realpath` + `startswith(<untainted root> + os.sep)` disjunction that dominates every filesystem operation (the canonical path-injection barrier); symlink escapes from the evidence directory are quarantined as binding violations and never read.

## 3. Evidence contract

Acceptance runs drop one JSON file per scenario into `release-readiness/evidence/` (gitignored):

```json
{ "candidate": "<must equal manifest.candidate>",
  "scenarios": { "<ID>": { "status": "pass", "platform": "linux-x64", "detail": "…" } } }
```

- `status` must be `pass` to count. Recognized non-passing structured statuses: `failed`, `skipped`, `not-run`, `pending`, `unknown`, `error`, `not-applicable`.
- **A `pass` never masks a failing result for the same requirement**: if any candidate-matched entry for `(scenario, platform)` is `failed`/`skipped`/`not-run`/`pending`/`unknown`/`error` while another is `pass`, the row resolves to **`conflict`** (with `conflicting_statuses` listed and every evidence source surfaced) and the overall verdict FAILS — both in a single entry list and across files. (`pass` + a preapproved `not-applicable` is not a conflict: the scenario ran and passed.)
- A scenario entry (or a list of entries) may carry `platform`; **platform-specific requirements are satisfied only by an entry whose `platform` matches** — an entry without a platform satisfies only `all_platforms` requirements.
- `not-applicable` counts ONLY if `(scenario, platform)` is preapproved in `not_applicable_scenarios`. An unapproved `not-applicable` result is itself a failure (`not-applicable-unapproved` bucket → overall FAIL) — N/A is never a run-time decision.
- A file is treated as scenario evidence iff it is a JSON object with a `scenarios` key; other JSON files in the dir are ignored. Files that are unreadable or lack a `candidate` are hard violations.

## 4. Aggregation semantics

`eval_manifest.aggregate(manifest, evidence_dir, expect_candidate=None)` produces the machine-generated verdict:

1. **Candidate binding (REJECT on mismatch).** Every scenario-evidence file whose `candidate` ≠ `manifest.candidate` is quarantined (none of its results count) and recorded as a binding violation → overall FAIL. `release_readiness.py validate --manifest …` **always** cross-checks the manifest candidate against the RESOLVED run candidate (explicit `--candidate`, else HEAD) — a manifest frozen for another candidate can never pass the gate; the standalone `eval_manifest.py aggregate` CLI enforces it only when `--candidate` is passed explicitly.
2. **Required-scenario coverage.** For every `all_platforms` ID and every `(platform, ID)` pair in `by_platform`: exactly a structured `pass` (or a preapproved `not-applicable`) counts. **missing / skipped / failed / not-run / unknown / pending → overall FAIL.**
3. **Verdict.** `ready` is true only when zero unmet rows, zero binding violations, no candidate mismatch. Emitted as JSON (per-scenario table included) and as a human-readable table. Exit codes: `0` ready / `1` not ready / `2` manifest misconfiguration.

```bash
python3 scripts/eval_manifest.py check --manifest eval-plan.json          # schema only
python3 scripts/eval_manifest.py aggregate --manifest eval-plan.json \
    --evidence-dir release-readiness/evidence [--candidate <token>] [--json] [--output verdict.json]
python3 scripts/eval_manifest.py self-test                                # negative controls
```

## 5. Negative controls (plan §10.2, enforced)

`eval_manifest.py self-test` proves the verdict logic with **real file round-trips**: each control writes a synthetic evidence dir to disk, then checks both the in-process aggregation and a real CLI subprocess (`exit code` + JSON verdict). Controls:

| Control | Setup | Required outcome |
|---|---|---|
| `NC-MAN-all-pass-passes` | both scenarios pass, candidate matches | READY, exit 0 |
| `NC-MAN-missing-scenario-fails` | required scenario absent from evidence | NOT READY, exit 1 |
| `NC-MAN-skipped-scenario-fails` | scenario status `skipped` | NOT READY, exit 1 |
| `NC-MAN-candidate-digest-mismatch-fails` | evidence bound to a different candidate digest | NOT READY, exit 1 |
| `NC-MAN-failed-rpc-fails-despite-later-health-pass` | scenario 1 `failed`, later health scenario `pass` | NOT READY, exit 1 |
| `NC-MAN-not-run-fails` | scenario status `not-run` | NOT READY, exit 1 |
| `NC-MAN-unapproved-not-applicable-fails` | scenario status `not-applicable` with no preapproval in the manifest | NOT READY, exit 1 |
| `NC-MAN-manifest-traversal-id-rejected` | manifest declares required scenario ID `../../etc/passwd` | schema-rejected, exit 2 (fail closed; ID never used as a path) |
| `NC-MAN-evidence-symlink-escape-rejected` | symlink inside the evidence dir points to a valid passing result outside it | symlink quarantined as binding violation, NOT READY, exit 1 |
| `NC-MAN-evidence-unsafe-scenario-id-rejected` | evidence file contains scenario key `../escape` with a passing result | binding violation, NOT READY, exit 1 |
| `NC-MAN-pass-failed-masking-conflict-fails` | one evidence file lists `failed` **and** `pass` entries for the same scenario | row = `conflict` (pass never masks failures), NOT READY, exit 1 |
| `NC-MAN-pass-failed-conflict-crossfile-fails` | `failed` and `pass` in two candidate-matched files for one scenario | row = `conflict`, both sources surfaced, NOT READY, exit 1 |
| `NC-MAN-empty-requirements-rejected` | manifest requires nothing (`all_platforms: []`, `by_platform: {"p": []}`) | schema-rejected, exit 2 — an empty gate is not a gate |
| `NC-MAN-cross-bucket-duplicate-rejected` | same scenario ID in `all_platforms` and a `by_platform` list | schema-rejected, exit 2 |
| `NC-MAN-newline-scenario-id-rejected` | required scenario ID `"S1-RPC\n"` (regex `$` anchor bypass) | schema-rejected, exit 2 (anchors use `\Z`) |
| `NC-MAN-stdin-invalid-json-exit-2` | invalid JSON piped to `--manifest -` | clean exit 2 misconfig, no traceback |

When `release_readiness.py` runs with `--manifest`, these controls execute as part of the gate and are added as required criteria `EVID-NC-MAN-*` alongside the aggregation verdict `EVID-MANIFEST` — so a broken aggregator fails `validate` too.

## 6. Integration with release_readiness.py

```bash
python3 scripts/release_readiness.py validate --manifest eval-plan.json \
    --candidate <token> --profile eval
```

- Adds `EVID-MANIFEST` (the aggregation verdict) and `EVID-NC-MAN-*` (controls) to the required set; `validate` exits 0 only if the 52 registry criteria, the engine controls, **and** the manifest aggregation all pass.
- `report --manifest …` shows the aggregation informationally without changing exit codes.
- Manifest load/schema failure → exit 2 (misconfiguration), consistent with repo convention.
- Without `--manifest`, behavior and output are byte-identical to the pre-manifest tool.

## 7. What the aggregator deliberately does NOT do

- It does not re-verify proto/OpenAPI hashes against the working tree — identity hashes are *recorded* here and *verified by the acceptance runs* (PROTO-*/PKG-* scenarios) whose structured evidence the aggregator ingests. Drift detectors remain labeled drift (checker plan §5).
- It does not orchestrate the heavy acceptance runs; it consumes their structured results (checker plan §8).
- It never accepts a hand-edited summary in place of a structured result (§12.5).
