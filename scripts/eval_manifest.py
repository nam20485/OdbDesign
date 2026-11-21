#!/usr/bin/env python3
"""eval-plan.json release manifest + acceptance-evidence aggregator for OdbDesign.

Implements plan section 10 (first-customer evaluation acceptance protocol):

  * ``eval-plan.json`` — the immutable release manifest (schema enforced here):
    release id/timestamps, candidate identity (upstream/commercial/client commits,
    ZIP sha256 / image digest), complete proto + OpenAPI hashes, supported platform
    matrix, network boundary, corpus aliases + sha256, resource envelope
    (memory/disk/timeouts), required scenario IDs per platform, and public
    signing-key fingerprints (NEVER private keys/passwords/license documents).
  * the aggregator — loads the manifest, ingests the structured scenario results
    the acceptance runs emit into ``release-readiness/evidence/<ID>.json``
    (shape ``{"candidate": ..., "scenarios": {"<ID>": {"status": "pass"|...}}}``),
    REJECTS any result whose candidate does not match the manifest candidate,
    requires every required scenario ID (per platform) to be present with a
    structured ``pass`` (missing/skipped/failed/not-run => overall FAIL), and
    emits a machine-generated overall verdict plus a per-scenario table.

Universal pass rules (plan 10.2) are enforced literally:
  - every required scenario ID must return structured `pass`;
  - `not-applicable` is accepted ONLY when preapproved in the manifest's
    ``not_applicable_scenarios`` — never as a run-time decision;
  - a candidate mismatch contaminates the bundle -> overall FAIL;
  - aggregator negative controls prove that a missing scenario, a skipped
    scenario, a changed candidate digest, or a failed RPC fails the overall
    verdict even when later health checks pass.

Exit codes (repo convention): 0 ok / 1 unmet (verdict FAIL, controls failed)
/ 2 misconfiguration (manifest missing/invalid).

stdlib only. Companion to scripts/release_readiness.py (``validate --manifest``).
"""

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_EVIDENCE_DIR = REPO_ROOT / "release-readiness" / "evidence"

MANIFEST_VERSION = 1
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
STATUS_PASS = "pass"
STATUS_NA = "not-applicable"
# Structured non-passing statuses recognized from evidence (plan 10.2):
# missing / Skipped / NotRun / failed all fail the required lane.
KNOWN_NON_PASS = ("failed", "skipped", "not-run", "pending", "unknown", "error")

SIGNING_KEY_ALLOWED_FIELDS = {"fingerprint", "purpose", "algorithm"}
SECRETISH_FIELD_RE = re.compile(r"private|secret|password|passwd|token|license[-_]?doc|credential", re.I)


class ManifestError(Exception):
    """Manifest missing/unreadable/invalid -> exit code 2 (misconfiguration)."""


# --------------------------------------------------------------------------- #
# Manifest schema validation
# --------------------------------------------------------------------------- #
def validate_manifest(m):
    """Return a list of schema problems ([] = valid). Never raises."""
    problems = []

    def req_str(obj, key, where, allow_empty=False):
        v = obj.get(key)
        if not isinstance(v, str) or (not allow_empty and not v.strip()):
            problems.append(f"{where}.{key}: required non-empty string")
        return v

    def req_sha(obj, key, where):
        v = req_str(obj, key, where)
        if isinstance(v, str) and not SHA256_RE.match(v):
            problems.append(f"{where}.{key}: not a lowercase 64-hex sha256: {v!r}")

    if not isinstance(m, dict):
        return ["manifest: top level must be a JSON object"]

    if m.get("manifest_version") != MANIFEST_VERSION:
        problems.append(f"manifest_version: must be {MANIFEST_VERSION}")
    req_str(m, "release_id", "manifest")
    req_str(m, "created_utc", "manifest")
    req_str(m, "candidate", "manifest")

    # Candidate identity (plan 10.1: commits, ZIP sha256, image digest, artifacts).
    identity = m.get("identity")
    if not isinstance(identity, dict):
        problems.append("identity: required object (candidate identity)")
    else:
        req_str(identity, "upstream_commit", "identity")
        for opt in ("commercial_commit", "test_harness_commit", "image_digest"):
            if identity.get(opt) is not None:
                req_str(identity, opt, "identity")
        clients = identity.get("client_commits")
        if clients is not None:
            if not isinstance(clients, dict) or not clients:
                problems.append("identity.client_commits: must be a non-empty object of {client: commit}")
            else:
                for name, commit in clients.items():
                    if not isinstance(commit, str) or not commit.strip():
                        problems.append(f"identity.client_commits.{name}: required non-empty commit")
        artifacts = identity.get("artifacts")
        if artifacts is not None:
            if not isinstance(artifacts, list) or not artifacts:
                problems.append("identity.artifacts: must be a non-empty list")
            else:
                for i, a in enumerate(artifacts):
                    if not isinstance(a, dict):
                        problems.append(f"identity.artifacts[{i}]: must be an object")
                        continue
                    req_str(a, "name", f"identity.artifacts[{i}]")
                    req_str(a, "kind", f"identity.artifacts[{i}]")
                    req_sha(a, "sha256", f"identity.artifacts[{i}]")

    # Complete proto hashes + OpenAPI source hash.
    protos = m.get("protos")
    if protos is not None:
        if not isinstance(protos, dict) or not protos:
            problems.append("protos: must be a non-empty object of {proto_path: sha256}")
        else:
            for p, h in protos.items():
                if not isinstance(h, str) or not SHA256_RE.match(h):
                    problems.append(f"protos.{p}: not a lowercase 64-hex sha256")
    openapi = m.get("openapi")
    if openapi is not None:
        if not isinstance(openapi, dict):
            problems.append("openapi: must be an object {source, sha256}")
        else:
            req_str(openapi, "source", "openapi")
            req_sha(openapi, "sha256", "openapi")

    # Supported platform matrix.
    matrix = m.get("support_matrix")
    if not isinstance(matrix, list) or not matrix:
        problems.append("support_matrix: required non-empty list of platform entries")
    else:
        for i, e in enumerate(matrix):
            if not isinstance(e, dict):
                problems.append(f"support_matrix[{i}]: must be an object")
                continue
            req_str(e, "platform", f"support_matrix[{i}]")
            for opt in ("os", "runtime", "hardware", "notes"):
                if opt in e and not isinstance(e[opt], str):
                    problems.append(f"support_matrix[{i}].{opt}: must be a string")

    # Network boundary.
    nb = m.get("network_boundary")
    if not isinstance(nb, dict) or not nb:
        problems.append("network_boundary: required non-empty object")

    # Corpus aliases + sha256 + privacy policy.
    corpus = m.get("corpus")
    if corpus is not None:
        if not isinstance(corpus, list):
            problems.append("corpus: must be a list of corpus entries")
        else:
            for i, c in enumerate(corpus):
                if not isinstance(c, dict):
                    problems.append(f"corpus[{i}]: must be an object")
                    continue
                req_str(c, "alias", f"corpus[{i}]")
                req_sha(c, "sha256", f"corpus[{i}]")
                if not isinstance(c.get("size_bytes"), int) or c.get("size_bytes") < 0:
                    problems.append(f"corpus[{i}].size_bytes: required non-negative integer")
                req_str(c, "privacy", f"corpus[{i}]")
                if "golden_version" in c and not isinstance(c["golden_version"], str):
                    problems.append(f"corpus[{i}].golden_version: must be a string")

    # Resource envelope (memory/disk limits, timeouts, perf envelope).
    env = m.get("resource_envelope")
    if not isinstance(env, dict):
        problems.append("resource_envelope: required object")
    else:
        for key in ("memory", "disk", "timeouts"):
            if key not in env or not isinstance(env[key], (dict, str, int, float)):
                problems.append(f"resource_envelope.{key}: required (object or scalar)")

    # Required scenario IDs per platform.
    rs = m.get("required_scenarios")
    if not isinstance(rs, dict):
        problems.append("required_scenarios: required object {all_platforms, by_platform}")
    else:
        allp = rs.get("all_platforms", [])
        byp = rs.get("by_platform", {})
        if not isinstance(allp, list) or not all(isinstance(s, str) and s.strip() for s in allp):
            problems.append("required_scenarios.all_platforms: must be a list of scenario ID strings")
            allp = []
        if not isinstance(byp, dict):
            problems.append("required_scenarios.by_platform: must be an object {platform: [ids]}")
            byp = {}
        else:
            for plat, ids in byp.items():
                if not isinstance(ids, list) or not all(isinstance(s, str) and s.strip() for s in ids):
                    problems.append(f"required_scenarios.by_platform.{plat}: must be a list of scenario IDs")
        if not allp and not byp:
            problems.append("required_scenarios: at least one required scenario ID overall")
        for lst, where in ((allp, "all_platforms"),):
            dupes = {s for s in lst if lst.count(s) > 1}
            if dupes:
                problems.append(f"required_scenarios.{where}: duplicate IDs: {sorted(dupes)}")
        if isinstance(byp, dict):
            for plat, ids in byp.items():
                if isinstance(ids, list):
                    dupes = {s for s in ids if ids.count(s) > 1}
                    if dupes:
                        problems.append(f"required_scenarios.by_platform.{plat}: duplicate IDs: {sorted(dupes)}")

    # Preapproved not-applicable entries (plan 10.2: support-matrix entry, not run-time).
    na = m.get("not_applicable_scenarios", [])
    if not isinstance(na, list):
        problems.append("not_applicable_scenarios: must be a list")
    else:
        for i, e in enumerate(na):
            if not isinstance(e, dict):
                problems.append(f"not_applicable_scenarios[{i}]: must be an object")
                continue
            req_str(e, "scenario", f"not_applicable_scenarios[{i}]")
            req_str(e, "reason", f"not_applicable_scenarios[{i}]")

    # Public signing-key fingerprints — never secrets (plan 12.5).
    keys = m.get("signing_keys")
    if not isinstance(keys, list):
        problems.append("signing_keys: required list of public fingerprint entries")
    else:
        for i, k in enumerate(keys):
            if not isinstance(k, dict):
                problems.append(f"signing_keys[{i}]: must be an object")
                continue
            extra = set(k) - SIGNING_KEY_ALLOWED_FIELDS
            if extra:
                problems.append(f"signing_keys[{i}]: only {sorted(SIGNING_KEY_ALLOWED_FIELDS)} fields allowed, got {sorted(extra)}")
            for field in k:
                if SECRETISH_FIELD_RE.search(field):
                    problems.append(f"signing_keys[{i}].{field}: secret-like field forbidden in manifest")
            req_str(k, "fingerprint", f"signing_keys[{i}]")

    return problems


def load_manifest(path):
    p = Path(path)
    if not p.exists():
        raise ManifestError(f"manifest not found: {p}")
    try:
        m = json.loads(p.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as e:
        raise ManifestError(f"manifest unreadable/invalid JSON: {e}")
    problems = validate_manifest(m)
    if problems:
        raise ManifestError("manifest failed schema validation:\n  - " + "\n  - ".join(problems))
    return m


# --------------------------------------------------------------------------- #
# Evidence ingestion + aggregation
# --------------------------------------------------------------------------- #
def _load_evidence(evidence_dir):
    """Read every *.json scenario-evidence file in the dir.

    A file is scenario evidence iff it is an object with a "scenarios" key.
    Returns (results, binding_violations) where results maps scenario ID ->
    list of (entry, source_file) and binding_violations is a list of
    {file, candidate, expected, scenarios_quarantined}.
    """
    results = {}
    violations = []
    d = Path(evidence_dir)
    files = sorted(d.glob("*.json")) if d.is_dir() else []
    for f in files:
        try:
            data = json.loads(f.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            results.setdefault("__unreadable__", []).append(({"status": "unknown"}, str(f)))
            continue
        if not isinstance(data, dict) or "scenarios" not in data:
            continue  # not scenario evidence (e.g. tool metadata); aggregator ignores it
        scenarios = data.get("scenarios")
        if not isinstance(scenarios, dict):
            violations.append({"file": str(f), "candidate": data.get("candidate"),
                               "expected": None, "reason": "scenarios key is not an object",
                               "scenarios_quarantined": 0})
            continue
        n_quarantined = len(scenarios)
        if not isinstance(data.get("candidate"), str) or not data.get("candidate"):
            violations.append({"file": str(f), "candidate": None, "expected": None,
                               "reason": "evidence file has no candidate binding",
                               "scenarios_quarantined": n_quarantined})
            continue
        # candidate equality is set by the caller via expect; here we only collect
        for scid, entry in scenarios.items():
            entries = entry if isinstance(entry, list) else [entry]
            for e in entries:
                if isinstance(e, dict):
                    results.setdefault(scid, []).append((e, str(f)))
                else:
                    results.setdefault(scid, []).append(({"status": "unknown", "raw": e}, str(f)))
    return results, violations, files


def _na_allowed(na_list, scid, platform):
    """Preapproved not-applicable lookup (plan 10.2)."""
    for n in na_list:
        if not isinstance(n, dict) or n.get("scenario") != scid:
            continue
        np = n.get("platform")
        if platform is None:
            if np in (None, "", "all", "*"):
                return n
        else:
            if np in (None, "", platform, "all", "*"):
                return n
    return None


def _resolve_row(scid, entries, platform, na_list):
    """Resolve one requirement (scid, platform-or-None).

    Returns (status, sources, na_reason, na_preapproved):
      status is one of: pass | not-applicable | failed | skipped | not-run |
      pending | unknown | error | missing.
      na_preapproved is True only when an NA result was accepted via a
      preapproved manifest entry; False when an NA result exists but nothing
      preapproves it (plan 10.2: N/A is never a run-time decision).
    """
    matching = [(e, f) for e, f in entries
                if platform is None or e.get("platform") == platform]
    if not matching:
        return "missing", [], None, None
    for e, f in matching:
        if e.get("status") == STATUS_PASS:
            return STATUS_PASS, [f], None, None
    for e, f in matching:
        if e.get("status") == STATUS_NA:
            na = _na_allowed(na_list, scid, platform)
            if na is not None:
                return STATUS_NA, [f], na.get("reason"), True
            return STATUS_NA, sorted({x for _, x in matching}), None, False
    status = matching[0][0].get("status")
    status = status if isinstance(status, str) else "unknown"
    if status not in KNOWN_NON_PASS and status not in (STATUS_PASS, STATUS_NA):
        status = "unknown"
    return status, sorted({f for _, f in matching}), None, None


def aggregate(manifest, evidence_dir, expect_candidate=None):
    """Produce the machine-generated aggregation verdict for one manifest."""
    candidate = manifest["candidate"]
    na_list = manifest.get("not_applicable_scenarios", [])
    rs = manifest["required_scenarios"]
    requirements = [(s, None) for s in rs.get("all_platforms", [])]
    byp = rs.get("by_platform", {}) or {}
    for plat, ids in byp.items():
        requirements.extend((s, plat) for s in (ids or []))

    verdict = {
        "kind": "eval-plan-aggregation",
        "release_id": manifest["release_id"],
        "candidate": candidate,
        "expect_candidate": expect_candidate,
        "evidence_dir": str(evidence_dir),
        "ready": False,
        "manifest_candidate_mismatch": None,
        "binding_violations": [],
        "rows": [],
        "counts": {},
    }

    if expect_candidate is not None and expect_candidate != candidate:
        verdict["manifest_candidate_mismatch"] = (
            f"--candidate {expect_candidate} != manifest candidate {candidate}"
        )

    results, violations, _files = _load_evidence(evidence_dir)

    # Candidate binding: evidence from ANY other candidate is rejected outright
    # (plan 10.2 "changed digest" control; EVID-02). Re-filter collected results
    # by re-reading candidates per file: _load_evidence collected before binding,
    # so enforce binding here by re-checking each source file.
    accepted_results = {}
    seen_files = {}
    for scid, entries in results.items():
        for e, f in entries:
            seen_files.setdefault(f, None)
    for f in seen_files:
        try:
            data = json.loads(Path(f).read_text(encoding="utf-8"))
            file_candidate = data.get("candidate") if isinstance(data, dict) else None
        except (json.JSONDecodeError, OSError):
            file_candidate = None
        seen_files[f] = file_candidate
    for scid, entries in results.items():
        for e, f in entries:
            fc = seen_files.get(f)
            if fc is None:
                violations.append({"file": f, "candidate": None, "expected": candidate,
                                   "reason": "unreadable or missing candidate binding",
                                   "scenarios_quarantined": 1})
            elif fc != candidate:
                violations.append({"file": f, "candidate": fc, "expected": candidate,
                                   "reason": "evidence candidate != manifest candidate (mixed candidates rejected)",
                                   "scenarios_quarantined": 1})
            else:
                accepted_results.setdefault(scid, []).append((e, f))

    # Deduplicate violations per file (a quarantined file is one violation).
    dedup = {}
    for v in violations:
        dedup[v["file"] + "|" + str(v.get("candidate")) + "|" + str(v.get("reason"))] = v
    verdict["binding_violations"] = sorted(dedup.values(), key=lambda v: v["file"])

    rows = []
    for scid, platform in sorted(requirements):
        status, sources, na_reason, na_pre = _resolve_row(scid, accepted_results.get(scid, []),
                                                          platform, na_list)
        req_label = "all" if platform is None else f"platform:{platform}"
        row = {"scenario": scid, "requirement": req_label, "status": status,
               "evidence": sources}
        if na_pre:
            row["not_applicable_reason"] = na_reason
        elif status == STATUS_NA:
            row["not_applicable_unapproved"] = True
        rows.append(row)

    counts = {"required_total": len(rows), STATUS_PASS: 0, STATUS_NA: 0,
              "not-applicable-unapproved": 0,
              "failed": 0, "skipped": 0, "not_run": 0, "missing": 0,
              "pending": 0, "unknown": 0, "error": 0}
    for r in rows:
        key = r["status"]
        if key == STATUS_NA and r.get("not_applicable_unapproved"):
            key = "not-applicable-unapproved"
        counts[key] = counts.get(key, 0) + 1
    counts["binding_violations"] = len(verdict["binding_violations"])
    verdict["counts"] = counts
    verdict["rows"] = rows

    unmet_rows = [r for r in rows
                  if r["status"] != STATUS_PASS and not (r["status"] == STATUS_NA and not r.get("not_applicable_unapproved"))]
    verdict["ready"] = (
        not unmet_rows
        and not verdict["binding_violations"]
        and verdict["manifest_candidate_mismatch"] is None
    )
    return verdict


# --------------------------------------------------------------------------- #
# Rendering
# --------------------------------------------------------------------------- #
def render_human(verdict):
    lines = []
    lines.append(f"eval-plan aggregation — release {verdict['release_id']} "
                 f"(candidate {verdict['candidate']})")
    lines.append(f"Evidence dir: {verdict['evidence_dir']}")
    c = verdict["counts"]
    lines.append(
        f"Required scenarios: {c['required_total']}  "
        f"pass={c[STATUS_PASS]}  not-applicable={c[STATUS_NA]}  "
        f"na-unapproved={c.get('not-applicable-unapproved', 0)}  "
        f"failed={c['failed']}  skipped={c['skipped']}  not-run={c['not_run']}  "
        f"missing={c['missing']}  unknown={c['unknown']}"
    )
    if verdict["manifest_candidate_mismatch"]:
        lines.append(f"CANDIDATE MISMATCH: {verdict['manifest_candidate_mismatch']}")
    if verdict["binding_violations"]:
        lines.append(f"Binding violations (rejected evidence): {len(verdict['binding_violations'])}")
        for v in verdict["binding_violations"]:
            lines.append(f"  {v['file']}: candidate={v.get('candidate')!r} "
                         f"expected={v.get('expected')!r} — {v['reason']}")
    lines.append("")
    width = max([len(r["scenario"]) for r in verdict["rows"]] + [7])
    for r in verdict["rows"]:
        src = ",".join(Path(f).name for f in r["evidence"]) if r["evidence"] else "—"
        if r.get("not_applicable_reason"):
            extra = f" (preapproved: {r['not_applicable_reason']})"
        elif r.get("not_applicable_unapproved"):
            extra = " (NOT preapproved in manifest -> fails the verdict)"
        else:
            extra = ""
        lines.append(f"  [{r['status']:<14}] {r['scenario']:<{width}}  {r['requirement']:<14}  {src}{extra}")
    lines.append("")
    lines.append("Verdict: READY" if verdict["ready"] else "Verdict: NOT READY")
    return "\n".join(lines)


# --------------------------------------------------------------------------- #
# Negative controls (real file round-trips through the real CLI)
# --------------------------------------------------------------------------- #
def _synthetic_manifest():
    """A minimal valid manifest for the controls (deterministic, hermetic)."""
    return {
        "manifest_version": 1,
        "release_id": "negative-control-release",
        "created_utc": "1970-01-01T00:00:00Z",
        "candidate": "cafe" * 8,
        "identity": {"upstream_commit": "b0b0b0b0"},
        "support_matrix": [{"platform": "linux-x64", "os": "debian-13"}],
        "network_boundary": {"rest": "loopback", "grpc": "loopback"},
        "resource_envelope": {"memory": "6GiB", "disk": "1GiB", "timeouts": {"upload_s": 300}},
        "required_scenarios": {"all_platforms": ["S1-RPC", "S2-HEALTH"]},
        "not_applicable_scenarios": [],
        "signing_keys": [{"fingerprint": "sha256:0000000000000000000000000000000000000000000000000000000000000000",
                          "purpose": "release-signing"}],
    }


def _write_evidence(path, candidate, scenarios):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"candidate": candidate, "scenarios": scenarios}, indent=2),
                    encoding="utf-8")


def run_negative_controls(self_exe=None):
    """Prove the verdict logic with REAL file round-trips through the real CLI.

    For each control: write a synthetic evidence dir to disk, run aggregation
    in-process AND via a real subprocess of this script, and check both the
    verdict field and the exit code. Returns [(control_id, ok, detail)].
    """
    self_exe = self_exe or str(Path(__file__).resolve())
    manifest = _synthetic_manifest()
    candidate = manifest["candidate"]
    other_candidate = "dead" * 8
    controls = []

    def cli_verdict(evidence_dir):
        p = subprocess.run(
            [sys.executable, "-I", self_exe, "aggregate",
             "--manifest", "-", "--evidence-dir", str(evidence_dir), "--json"],
            input=json.dumps(manifest), capture_output=True, text=True, timeout=60,
        )
        return p.returncode, p.stdout

    def check(control_id, scenarios, expect_ready, expect_exit, cand=candidate):
        with tempfile.TemporaryDirectory(prefix="evalman-nc-") as td:
            ev = Path(td) / "evidence"
            _write_evidence(ev / "control.json", cand, scenarios)
            v = aggregate(manifest, ev)
            ok_verdict = v["ready"] == expect_ready
            rc, out = cli_verdict(ev)
            ok_exit = rc == expect_exit
            ok_json = True
            try:
                j = json.loads(out)
                if j.get("ready") != expect_ready:
                    ok_json = False
            except json.JSONDecodeError:
                ok_json = False
            ok = ok_verdict and ok_exit and ok_json
            detail = (f"in-process ready={v['ready']} (want {expect_ready}), "
                      f"CLI exit={rc} (want {expect_exit}), CLI json ready-OK={ok_json}")
            controls.append((control_id, ok, detail))

    check("NC-MAN-all-pass-passes",
          {"S1-RPC": {"status": "pass"}, "S2-HEALTH": {"status": "pass"}},
          expect_ready=True, expect_exit=0)
    check("NC-MAN-missing-scenario-fails",
          {"S1-RPC": {"status": "pass"}},
          expect_ready=False, expect_exit=1)
    check("NC-MAN-skipped-scenario-fails",
          {"S1-RPC": {"status": "pass"}, "S2-HEALTH": {"status": "skipped"}},
          expect_ready=False, expect_exit=1)
    check("NC-MAN-candidate-digest-mismatch-fails",
          {"S1-RPC": {"status": "pass"}, "S2-HEALTH": {"status": "pass"}},
          expect_ready=False, expect_exit=1, cand=other_candidate)
    check("NC-MAN-failed-rpc-fails-despite-later-health-pass",
          {"S1-RPC": {"status": "failed"}, "S2-HEALTH": {"status": "pass"}},
          expect_ready=False, expect_exit=1)
    check("NC-MAN-not-run-fails",
          {"S1-RPC": {"status": "pass"}, "S2-HEALTH": {"status": "not-run"}},
          expect_ready=False, expect_exit=1)
    check("NC-MAN-unapproved-not-applicable-fails",
          {"S1-RPC": {"status": "pass"}, "S2-HEALTH": {"status": "not-applicable"}},
          expect_ready=False, expect_exit=1)
    return controls


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #
def _read_manifest_arg(args):
    if args.manifest == "-":
        m = json.load(sys.stdin)
    else:
        m = load_manifest(args.manifest)
    problems = validate_manifest(m)
    if problems:
        raise ManifestError("manifest failed schema validation:\n  - " + "\n  - ".join(problems))
    return m


def main(argv=None):
    ap = argparse.ArgumentParser(description="eval-plan.json manifest + evidence aggregator")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p_agg = sub.add_parser("aggregate", help="aggregate evidence against a manifest")
    p_agg.add_argument("--manifest", required=True,
                       help="path to eval-plan.json ('-' reads it from stdin)")
    p_agg.add_argument("--evidence-dir", default=str(DEFAULT_EVIDENCE_DIR))
    p_agg.add_argument("--candidate", default=None,
                       help="also require the manifest candidate to equal this")
    p_agg.add_argument("--json", action="store_true", help="machine-readable verdict only")
    p_agg.add_argument("--output", default=None, help="also write the verdict JSON here")

    p_chk = sub.add_parser("check", help="validate manifest schema only")
    p_chk.add_argument("--manifest", required=True)

    sub.add_parser("self-test", help="run aggregator negative controls (real round-trips)")

    args = ap.parse_args(argv)

    if args.cmd == "self-test":
        controls = run_negative_controls()
        failed = 0
        print("Aggregator negative controls (real file round-trips via CLI):")
        for cid, ok, detail in controls:
            print(f"  [{'PASS' if ok else 'FAIL'}] {cid}: {detail}")
            if not ok:
                failed += 1
        print(f"{len(controls) - failed}/{len(controls)} controls behave as required")
        return 1 if failed else 0

    if args.cmd == "check":
        try:
            load_manifest(args.manifest)
        except ManifestError as e:
            print(f"INVALID manifest: {e}", file=sys.stderr)
            return 2
        print(f"manifest OK: {args.manifest}")
        return 0

    # aggregate
    try:
        manifest = _read_manifest_arg(args)
    except ManifestError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2
    verdict = aggregate(manifest, args.evidence_dir, expect_candidate=args.candidate)
    if args.output:
        out = Path(args.output)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(verdict, indent=2) + "\n", encoding="utf-8")
    if args.json:
        print(json.dumps(verdict, indent=2))
    else:
        print(render_human(verdict))
    return 0 if verdict["ready"] else 1


if __name__ == "__main__":
    sys.exit(main())
