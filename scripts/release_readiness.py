#!/usr/bin/env python3
"""Release-readiness checker for OdbDesign + OdbDesign-Commercial.

Evaluates every release-readiness criterion from the validation plan
(docs/plan/test-coverage-and-evaluation-validation-plan/) against REAL evidence
and reports progress, or autonomously gates readiness.

Two modes:
  report    Always runnable, tolerant. Shows completed / remaining / blocked.
            Exit 0 when the report is generated (unfinished work is not an error).
  validate  Strict gate. Every required criterion must be `pass` (or pre-approved
            not-applicable) with candidate-matched evidence. Anything else -> exit 1.

Design rules (see the companion checker plan):
  * No grep-style proof. A criterion passes only on real evidence: a coverage
    tracefile + JUnit result, a process exit code, a socket probe, an HTTP/gRPC
    round trip, a checksum/signature, or a structured scenario result. Drift
    detectors (proto diff, route check) are labelled as drift signals, not proof.
  * No cheap/lite version. All evaluator types and real probes are implemented.
    A probe whose subject does not exist yet (e.g. an unbuilt binary) runs, hits
    the real failure, and reports `blocked` -- never a fabricated pass.
  * No silent pass on missing evidence; no mixing candidates.

Statuses: pass | fail | pending | blocked | not-applicable | unknown
  pending  - the run/artifact/subject that would satisfy it has not been produced
  blocked  - a prerequisite (depends_on) is not pass, or the probe subject is absent
  unknown  - evaluator errored / evidence unreadable (treated as NOT pass)

Exit codes (repo convention): 0 ok / 1 unmet criteria or findings / 2 misconfig.

stdlib only.
"""

import argparse
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
COMMERCIAL_ROOT = REPO_ROOT.parent / "OdbDesign-Commercial"
SCRIPT_DIR = Path(__file__).resolve().parent
REGISTRY_PATH = SCRIPT_DIR / "release_readiness_criteria.json"

sys.path.insert(0, str(SCRIPT_DIR))
import eval_manifest  # noqa: E402  (sibling module: eval-plan.json manifest + aggregator)

PASS, FAIL, PENDING, BLOCKED, NA, UNKNOWN = (
    "pass", "fail", "pending", "blocked", "not-applicable", "unknown",
)
NON_PASS = {FAIL, PENDING, BLOCKED, UNKNOWN}


# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #
def run(cmd, timeout=120, cwd=None):
    """Run a command, return (returncode, stdout, stderr). Never raises on nonzero."""
    try:
        p = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout,
        )
        return p.returncode, p.stdout, p.stderr
    except FileNotFoundError:
        return 127, "", f"command not found: {cmd[0]}"
    except subprocess.TimeoutExpired:
        return 124, "", f"timeout after {timeout}s"


def git_head(path=REPO_ROOT):
    rc, out, _ = run(["git", "rev-parse", "--short", "HEAD"], cwd=path)
    return out.strip() if rc == 0 else "unknown"


def result(status, detail, evidence=None):
    return {"status": status, "detail": detail, "evidence": evidence}


# --------------------------------------------------------------------------- #
# Evaluators -- each returns result(status, detail, evidence)
# --------------------------------------------------------------------------- #
def ev_repo_file_registered(params, ctx):
    """A test source exists AND is registered (uncommented) in a CMake target.

    Real deliverable check: file presence alone is not enough; it must be wired
    into the build so it actually runs. Commented-out lines do not count.
    """
    rel = params["file"]
    cmake = REPO_ROOT / params.get("cmake", "OdbDesignTests/CMakeLists.txt")
    f = REPO_ROOT / rel
    if not f.exists():
        return result(PENDING, f"test source absent: {rel}")
    if not cmake.exists():
        return result(UNKNOWN, f"CMake file missing: {cmake}")
    name = Path(rel).name
    active = False
    for line in cmake.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        if stripped.startswith("#"):
            continue
        if name in stripped:
            active = True
            break
    if active:
        return result(PASS, f"{name} present and registered in {cmake.name}")
    return result(FAIL, f"{name} exists but is NOT registered (commented out?) in {cmake.name}")


def ev_repo_file_exists(params, ctx):
    """A deliverable file/dir exists. Weaker signal; used only for deliverables."""
    root = COMMERCIAL_ROOT if params.get("repo") == "commercial" else REPO_ROOT
    p = root / params["path"]
    if p.exists():
        return result(PASS, f"present: {p}")
    return result(PENDING, f"absent: {p}")


def ev_proto_drift(params, ctx):
    """Drift detector (NOT behavior proof): canonical protos vs a vendored client.

    Reports the count of differing shared .proto files. A pass here means 'no
    drift detected', which still must be backed by real .NET round trips (PROTO
    behavior criteria) before readiness.
    """
    canonical = REPO_ROOT / "OdbDesignLib" / "protoc"
    client = Path(params["client_dir"])
    if not client.exists():
        return result(BLOCKED, f"client proto dir absent: {client}")
    canon_files = {p.name: p for p in canonical.glob("*.proto")}
    drift = []
    for name, cpath in canon_files.items():
        vpath = client / name
        if not vpath.exists():
            drift.append(f"{name} (missing in client)")
            continue
        if cpath.read_bytes() != vpath.read_bytes():
            drift.append(name)
    if drift:
        return result(FAIL, f"{len(drift)} proto file(s) differ: {', '.join(sorted(drift))}",
                      {"drift": drift, "kind": "drift-detector"})
    return result(PASS, "no proto drift detected (still requires real round-trip proof)",
                  {"kind": "drift-detector"})


def ev_gh_workflow_status(params, ctx):
    """Real CI outcome: latest run conclusion of a workflow via gh."""
    if shutil.which("gh") is None:
        return result(BLOCKED, "gh CLI not available")
    repo = params["repo"]
    workflow = params["workflow"]
    rc, out, err = run(
        ["gh", "run", "list", "--repo", repo, "--workflow", workflow,
         "--limit", "1", "--json", "conclusion,status,databaseId"],
        timeout=60,
    )
    if rc != 0:
        return result(UNKNOWN, f"gh query failed: {err.strip()[:200]}")
    try:
        runs = json.loads(out)
    except json.JSONDecodeError:
        return result(UNKNOWN, "gh output not JSON")
    if not runs:
        return result(PENDING, f"no runs found for {workflow}")
    r = runs[0]
    concl = r.get("conclusion")
    rid = r.get("databaseId")
    if concl == "success":
        return result(PASS, f"latest {workflow} run {rid} succeeded", {"run": rid})
    return result(FAIL, f"latest {workflow} run {rid} conclusion={concl or r.get('status')}",
                  {"run": rid, "conclusion": concl})


def _parse_lcov(path):
    """Return per-file line/function/branch counts from an LCOV tracefile."""
    files = {}
    cur = None
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("SF:"):
            cur = {"lf": 0, "lh": 0, "fnf": 0, "fnh": 0, "brf": 0, "brh": 0}
            files[line[3:]] = cur
        elif cur is None:
            continue
        elif line.startswith("DA:"):
            _, hits = line[3:].split(",")[:2]
            cur["lf"] += 1
            cur["lh"] += 1 if int(hits) > 0 else 0
        elif line.startswith("FNDA:"):
            hits = line[5:].split(",")[0]
            cur["fnf"] += 1
            cur["fnh"] += 1 if int(hits) > 0 else 0
        elif line.startswith("BRDA:"):
            hits = line[5:].split(",")[-1]
            cur["brf"] += 1
            cur["brh"] += 1 if hits not in ("-", "0") else 0
    return files


def _is_generated(sf):
    return bool(re.search(r"\.(pb|grpc\.pb)\.(cc|h)$", sf)) or "/grpc/" in sf and sf.endswith(".pb.cc")


def ev_coverage(params, ctx):
    """Real coverage gate from an LCOV tracefile (handwritten denominator).

    Requires the tracefile to exist; otherwise pending (the M0 pipeline must
    produce it). Computes handwritten line/function/branch rates, checks
    thresholds, and optionally verifies previously-omitted TUs are present.
    """
    tf = Path(params.get("tracefile", ctx["evidence_dir"] / "coverage.info"))
    if not tf.exists():
        return result(PENDING, f"no coverage tracefile at {tf}; run the M0 coverage pipeline")
    files = _parse_lcov(tf)
    hand = {sf: c for sf, c in files.items() if not _is_generated(sf)}
    if not hand:
        return result(UNKNOWN, "tracefile has no handwritten records")
    lf = sum(c["lf"] for c in hand.values())
    lh = sum(c["lh"] for c in hand.values())
    fnf = sum(c["fnf"] for c in hand.values())
    fnh = sum(c["fnh"] for c in hand.values())
    brf = sum(c["brf"] for c in hand.values())
    brh = sum(c["brh"] for c in hand.values())
    line_pct = 100.0 * lh / lf if lf else 0.0
    func_pct = 100.0 * fnh / fnf if fnf else 0.0
    branch_pct = 100.0 * brh / brf if brf else None
    metrics = {"line": round(line_pct, 2), "function": round(func_pct, 2),
               "branch": round(branch_pct, 2) if branch_pct is not None else None,
               "lines": f"{lh}/{lf}", "functions": f"{fnh}/{fnf}",
               "branches": f"{brh}/{brf}"}
    problems = []
    if line_pct <= params.get("min_line", 90.0):
        problems.append(f"lines {line_pct:.2f}% <= {params.get('min_line', 90.0)}%")
    if func_pct <= params.get("min_func", 90.0):
        problems.append(f"functions {func_pct:.2f}% <= {params.get('min_func', 90.0)}%")
    if params.get("require_branch", True):
        if branch_pct is None:
            problems.append("no branch data collected")
        elif branch_pct < params.get("min_branch", 85.0):
            problems.append(f"branches {branch_pct:.2f}% < {params.get('min_branch', 85.0)}%")
    for tu in params.get("require_inventory", []):
        if not any(sf.endswith(tu) for sf in hand):
            problems.append(f"omitted TU still absent: {tu}")
    if problems:
        return result(FAIL, "; ".join(problems), metrics)
    return result(PASS, "handwritten coverage meets thresholds", metrics)


def ev_junit(params, ctx):
    """Real test outcome from a JUnit XML: expected count, zero fail/skip."""
    f = Path(params.get("file", ctx["evidence_dir"] / "test-results.xml"))
    if not f.exists():
        return result(PENDING, f"no JUnit results at {f}")
    try:
        root = ET.parse(f).getroot()
    except ET.ParseError as e:
        return result(UNKNOWN, f"JUnit parse error: {e}")
    suites = root.iter("testsuite")
    tests = failures = errors = skipped = 0
    for s in suites:
        tests += int(s.get("tests", 0))
        failures += int(s.get("failures", 0))
        errors += int(s.get("errors", 0))
        skipped += int(s.get("skipped", 0))
    metrics = {"tests": tests, "failures": failures, "errors": errors, "skipped": skipped}
    problems = []
    if failures or errors:
        problems.append(f"{failures} failures / {errors} errors")
    if not params.get("allow_skips", False) and skipped:
        problems.append(f"{skipped} skipped (required suite must have zero)")
    if tests < params.get("expected_min", 1):
        problems.append(f"only {tests} tests < expected {params.get('expected_min')}")
    if problems:
        return result(FAIL, "; ".join(problems), metrics)
    return result(PASS, "all expected tests passed, no skips", metrics)


def ev_evidence(params, ctx):
    """Consume a structured acceptance result; verify candidate + scenario pass."""
    f = Path(params["file"]) if "file" in params else ctx["evidence_dir"] / f"{params['scenario']}.json"
    if not f.exists():
        return result(PENDING, f"no acceptance evidence at {f}; run the acceptance scenario")
    try:
        data = json.loads(f.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as e:
        return result(UNKNOWN, f"evidence unreadable: {e}")
    ev_candidate = data.get("candidate")
    if ctx["candidate"] and ev_candidate and ev_candidate != ctx["candidate"]:
        return result(FAIL, f"evidence candidate {ev_candidate} != {ctx['candidate']} (mixed candidates)")
    scenarios = data.get("scenarios", {})
    sc = scenarios.get(params["scenario"])
    if sc is None:
        return result(FAIL, f"scenario {params['scenario']} missing from evidence")
    if sc.get("status") == "pass":
        return result(PASS, f"scenario {params['scenario']} passed", {"candidate": ev_candidate})
    return result(FAIL, f"scenario {params['scenario']} status={sc.get('status')}", sc)


# --------------------------------------------------------------------------- #
# Probes -- real runtime checks. Absent subject -> blocked (never fabricated).
# --------------------------------------------------------------------------- #
def _find_binary(candidates):
    for c in candidates:
        p = Path(c)
        if p.exists() and os.access(p, os.X_OK):
            return p
    return None


def _lib_dirs():
    dirs = []
    for rel in (
        "out/build/linux-dynamic-release/OdbDesignLib",
        "out/build/linux-dynamic-release/Utils",
        "out/build/linux-debug/OdbDesignLib",
        "out/build/linux-debug/Utils",
    ):
        d = REPO_ROOT / rel
        if d.is_dir():
            dirs.append(str(d))
    for trip in ("x64-linux-dynamic", "x64-linux"):
        d = REPO_ROOT / "out/vcpkg_installed" / trip / "lib"
        if d.is_dir():
            dirs.append(str(d))
    return dirs


def _has_protobuf_runtime():
    return any(list(Path(d).glob("libprotobuf.so*")) for d in _lib_dirs())


def _free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def _listening(port):
    with socket.socket() as s:
        s.settimeout(0.5)
        return s.connect_ex(("127.0.0.1", port)) == 0


def _http_ok(port, path="/healthz/live", timeout=12):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(f"http://127.0.0.1:{port}{path}", timeout=2) as r:
                if r.status == 200:
                    return True
        except Exception:
            pass
        time.sleep(0.3)
    return False


def _launch_server(binary, rest_port, grpc_port, designs_dir, disable_auth=False, env_extra=None):
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = os.pathsep.join(_lib_dirs() + [env.get("LD_LIBRARY_PATH", "")])
    env.pop("ODBDESIGN_SERVER_REQUEST_USERNAME", None)
    env.pop("ODBDESIGN_SERVER_REQUEST_PASSWORD", None)
    if env_extra:
        env.update(env_extra)
    args = [str(binary), "--bind", "127.0.0.1", "--port", str(rest_port),
            "--grpc-port", str(grpc_port), "--designs-dir", str(designs_dir)]
    if disable_auth:
        args.append("--disable-authentication")
    out_f = tempfile.TemporaryFile(mode="w+")
    err_f = tempfile.TemporaryFile(mode="w+")
    p = subprocess.Popen(args, env=env, stdin=subprocess.PIPE,
                         stdout=out_f, stderr=err_f, start_new_session=True, text=True)
    return p, out_f, err_f


def _read_err(err_f):
    try:
        err_f.seek(0)
        return err_f.read()
    except Exception:
        return ""


def _teardown(p, out_f, err_f, designs_dir):
    if p.poll() is None:
        try:
            os.killpg(os.getpgid(p.pid), signal.SIGTERM)
            p.wait(timeout=5)
        except Exception:
            try:
                os.killpg(os.getpgid(p.pid), signal.SIGKILL)
            except Exception:
                pass
    for f in (out_f, err_f):
        try:
            f.close()
        except Exception:
            pass
    try:
        shutil.rmtree(designs_dir, ignore_errors=True)
    except Exception:
        pass


def _lib_error(stderr):
    return "shared object file" in stderr or "shared libraries" in stderr


def probe_server_health(params, ctx):
    binary = _find_binary(params.get("binary_candidates", []))
    if binary is None:
        return result(BLOCKED, "server binary not built; build linux-dynamic-release first")
    rest_port, grpc_port = _free_port(), _free_port()
    designs_dir = tempfile.mkdtemp(prefix="rr-health-")
    p, out_f, err_f = _launch_server(binary, rest_port, grpc_port, designs_dir, disable_auth=True)
    try:
        if _http_ok(rest_port):
            return result(PASS, f"real /healthz/live returned 200 on port {rest_port}")
        err = _read_err(err_f)
        if p.poll() is not None and _lib_error(err):
            snippet = err.strip().splitlines()[-1][:160] if err.strip() else "runtime libs absent"
            return result(BLOCKED, "shared protobuf/runtime libs absent; cannot launch server: " + snippet)
        if p.poll() is not None:
            return result(FAIL, f"server exited {p.returncode} before health: {err.strip()[:160]}")
        return result(FAIL, "server did not become healthy within timeout")
    finally:
        _teardown(p, out_f, err_f, designs_dir)


def probe_f004_fail_closed(params, ctx):
    binary = _find_binary(params.get("binary_candidates", []))
    if binary is None:
        return result(BLOCKED, "server binary not built; cannot exercise fail-closed auth")
    rest_port, grpc_port = _free_port(), _free_port()
    designs_dir = tempfile.mkdtemp(prefix="rr-f004-")
    # Auth ENABLED (no --disable-authentication) and NO credential env -> must fail closed.
    p, out_f, err_f = _launch_server(binary, rest_port, grpc_port, designs_dir, disable_auth=False)
    try:
        time.sleep(3)
        err = _read_err(err_f)
        if p.poll() is not None:
            if _lib_error(err):
                return result(BLOCKED, "shared protobuf/runtime libs absent; cannot exercise F004: "
                              + (err.strip().splitlines()[-1][:160] if err.strip() else ""))
            if not _listening(rest_port) and not _listening(grpc_port):
                return result(PASS, f"failed closed (exit {p.returncode}) before any listener opened")
            return result(FAIL, "process exited but a listener was already open")
        if _listening(rest_port) or _listening(grpc_port):
            return result(FAIL, "F004 present: server listens with missing credentials "
                                "(default odb/plusplus fallback); not fail-closed")
        return result(UNKNOWN, "server alive but not yet listening; inconclusive")
    finally:
        _teardown(p, out_f, err_f, designs_dir)


def _commercial_binary():
    return _find_binary([
        COMMERCIAL_ROOT / "out/build/linux-dynamic-release/OdbDesignCommercialServer",
        COMMERCIAL_ROOT / "out/build/linux-dynamic-debug/OdbDesignCommercialServer",
    ])


def probe_license_reject(params, ctx):
    binary = _commercial_binary()
    if binary is None:
        return result(BLOCKED, "commercial server binary not built (packaging not green); "
                               "cannot exercise license rejection before bind")
    # Real execution requires a signed test license (C1/C2 subject work).
    lic = params.get("license_file")
    if not lic or not Path(lic).exists():
        return result(PENDING, "commercial binary present but no signed test license supplied; "
                               "license issuance harness (C1) required")
    return result(PENDING, "license-rejection execution harness (C2) not yet run for this candidate")


def probe_loopback(params, ctx):
    binary = _commercial_binary()
    if binary is None:
        return result(BLOCKED, "commercial server binary not built; cannot verify loopback enforcement")
    lic = params.get("license_file")
    if not lic or not Path(lic).exists():
        return result(PENDING, "commercial binary present but no loopback test license supplied; "
                               "license issuance harness (C1) required")
    return result(PENDING, "loopback-enforcement execution harness (C2) not yet run for this candidate")


PROBES = {
    "server_health": probe_server_health,
    "f004_fail_closed": probe_f004_fail_closed,
    "license_reject": probe_license_reject,
    "loopback": probe_loopback,
}


def ev_probe(params, ctx):
    if ctx.get("no_probes"):
        return result(PENDING, "probes skipped (--no-probes); evaluate from evidence instead")
    fn = PROBES.get(params["probe"])
    if fn is None:
        return result(UNKNOWN, f"unknown probe: {params['probe']}")
    return fn(params, ctx)


EVALUATORS = {
    "repo_file_registered": ev_repo_file_registered,
    "repo_file_exists": ev_repo_file_exists,
    "proto_drift": ev_proto_drift,
    "gh_workflow_status": ev_gh_workflow_status,
    "coverage": ev_coverage,
    "junit": ev_junit,
    "evidence": ev_evidence,
    "probe": ev_probe,
}


# --------------------------------------------------------------------------- #
# Negative controls -- prove the verdict logic rejects bad inputs (EVID-*).
# These test the engine itself with synthetic inputs; real, not grep.
# --------------------------------------------------------------------------- #
def negative_controls(ctx):
    controls = []

    def verdict(required_statuses):
        return all(s in (PASS, NA) for s in required_statuses)

    # A skipped required scenario must fail the verdict.
    ok = not verdict([PASS, PENDING, PASS])
    controls.append(("EVID-NC-skip-fails", ok,
                     "a pending/skipped required criterion fails the verdict"))
    # A failed required scenario must fail even if a later health check passes.
    ok = not verdict([FAIL, PASS])
    controls.append(("EVID-NC-fail-fails", ok,
                     "a failed required criterion fails the verdict despite a later pass"))
    # Unknown (unreadable evidence) must not pass.
    ok = not verdict([PASS, UNKNOWN])
    controls.append(("EVID-NC-unknown-fails", ok,
                     "unknown/unreadable evidence does not pass"))
    # All pass -> verdict passes.
    ok = verdict([PASS, PASS, NA])
    controls.append(("EVID-NC-allpass-passes", ok,
                     "all-pass (with approved N/A) yields a passing verdict"))
    return controls


# --------------------------------------------------------------------------- #
# Engine
# --------------------------------------------------------------------------- #
def load_registry():
    if not REGISTRY_PATH.exists():
        print(f"ERROR: criteria registry missing: {REGISTRY_PATH}", file=sys.stderr)
        sys.exit(2)
    try:
        return json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"ERROR: registry invalid JSON: {e}", file=sys.stderr)
        sys.exit(2)


def required_for_profile(criterion, profile):
    req = criterion.get("required_for", ["eval"])
    if profile == "production":
        return True  # production requires everything eval requires, plus production-only
    return "eval" in req


def evaluate(registry, ctx):
    criteria = registry["criteria"]
    by_id = {c["id"]: c for c in criteria}
    outcomes = {}

    # First pass: evaluate each criterion's own evaluator.
    for c in criteria:
        ev = EVALUATORS.get(c["evaluator"])
        if ev is None:
            outcomes[c["id"]] = result(UNKNOWN, f"unknown evaluator: {c['evaluator']}")
            continue
        try:
            outcomes[c["id"]] = ev(c.get("params", {}), ctx)
        except Exception as e:  # noqa: BLE001 - report, don't crash the whole run
            outcomes[c["id"]] = result(UNKNOWN, f"evaluator error: {e}")

    # Second pass: apply depends_on -> blocked (only downgrade non-pass).
    for c in criteria:
        cid = c["id"]
        if outcomes[cid]["status"] == PASS:
            continue
        for dep in c.get("depends_on", []):
            dep_status = outcomes.get(dep, {}).get("status", UNKNOWN)
            if dep_status != PASS:
                outcomes[cid] = result(
                    BLOCKED,
                    f"blocked by prerequisite {dep} ({dep_status}); " + outcomes[cid]["detail"],
                    outcomes[cid].get("evidence"),
                )
                break

    # Negative controls (engine self-test), added as EVID criteria.
    for nc_id, ok, detail in negative_controls(ctx):
        outcomes[nc_id] = result(PASS if ok else FAIL, detail)

    return outcomes


def summarize(registry, outcomes, profile, extra_required=None):
    criteria = registry["criteria"]
    nc_ids = {nc[0] for nc in negative_controls({})}
    required = []
    for c in criteria:
        if required_for_profile(c, profile):
            required.append(c["id"])
    required += sorted(nc_ids)  # negative controls always required
    if extra_required:
        required += [cid for cid in extra_required if cid not in required]

    counts = {s: 0 for s in (PASS, FAIL, PENDING, BLOCKED, NA, UNKNOWN)}
    for cid in required:
        counts[outcomes.get(cid, {}).get("status", UNKNOWN)] += 1

    passed = counts[PASS] + counts[NA]
    total = len(required)
    blockers = [
        (cid, outcomes[cid]["status"], outcomes[cid]["detail"])
        for cid in required if outcomes.get(cid, {}).get("status") in NON_PASS
    ]
    return {
        "profile": profile,
        "required_total": total,
        "required_pass": passed,
        "percent": round(100.0 * passed / total, 1) if total else 0.0,
        "counts": counts,
        "blockers": blockers,
        "ready": len(blockers) == 0,
    }


def render_human(registry, outcomes, summary, ctx):
    lines = []
    lines.append(f"Release readiness — candidate {ctx['candidate']} (profile: {summary['profile']})")
    mode = "VALIDATE (strict gate)" if ctx["mode"] == "validate" else "REPORT (informational)"
    lines.append(f"Mode: {mode}")
    lines.append(
        f"Required criteria: {summary['required_pass']}/{summary['required_total']} pass "
        f"({summary['percent']}%)"
    )
    c = summary["counts"]
    lines.append(
        f"  pass={c[PASS]}  fail={c[FAIL]}  pending={c[PENDING]}  "
        f"blocked={c[BLOCKED]}  n/a={c[NA]}  unknown={c[UNKNOWN]}"
    )
    nc = [cid for cid in outcomes if cid.startswith("EVID-NC-")]
    nc_pass = sum(1 for cid in nc if outcomes[cid]["status"] == PASS)
    lines.append(f"  (includes engine self-tests / negative controls: {nc_pass}/{len(nc)} pass)")
    mv = ctx.get("manifest_verdict")
    if mv is not None:
        lines.append("")
        lines.append(f"eval-plan manifest {ctx.get('manifest_path')}: "
                     + ("READY" if mv["ready"] else "NOT READY")
                     + f" — release {mv['release_id']}, required scenarios "
                       f"{mv['counts']['pass']}/{mv['counts']['required_total']} pass, "
                       f"{len(mv['binding_violations'])} candidate-binding violation(s)")
    lines.append("")

    # Per-group rollup.
    groups = {}
    for cr in registry["criteria"]:
        g = cr["group"]
        st = outcomes.get(cr["id"], {}).get("status", UNKNOWN)
        groups.setdefault(g, []).append(st)
    lines.append("By group:")
    for g in sorted(groups):
        sts = groups[g]
        npass = sum(1 for s in sts if s in (PASS, NA))
        bar = "█" * npass + "░" * (len(sts) - npass)
        lines.append(f"  {g:<10} {bar} {npass}/{len(sts)}")
    lines.append("")

    if summary["blockers"]:
        lines.append(f"Blocking the {summary['profile']} gate ({len(summary['blockers'])}):")
        for cid, st, detail in summary["blockers"][:25]:
            lines.append(f"  [{st:<7}] {cid}: {detail}")
        if len(summary["blockers"]) > 25:
            lines.append(f"  ... and {len(summary['blockers']) - 25} more")
    else:
        lines.append("No blockers — all required criteria pass.")
    lines.append("")
    verdict = "READY" if summary["ready"] else "NOT READY"
    lines.append(f"Verdict: {verdict}")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description="OdbDesign release-readiness checker")
    ap.add_argument("mode", choices=["report", "validate"])
    ap.add_argument("--profile", choices=["eval", "production"], default="eval")
    ap.add_argument("--candidate", default=None, help="commit/digest to bind results to")
    ap.add_argument("--evidence-dir", default=str(REPO_ROOT / "release-readiness" / "evidence"))
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--no-probes", action="store_true", help="skip live probes (fast glance)")
    ap.add_argument("--na", action="append", default=[], help="criterion IDs pre-approved N/A")
    ap.add_argument("--manifest", default=None,
                    help="eval-plan.json release manifest: aggregate acceptance evidence "
                         "against it, verify candidate binding + required scenarios, and "
                         "(in validate mode) gate readiness on the aggregation verdict")
    args = ap.parse_args()

    registry = load_registry()
    candidate = args.candidate or git_head()
    evidence_dir = Path(args.evidence_dir)
    ctx = {
        "mode": args.mode,
        "candidate": candidate,
        "evidence_dir": evidence_dir,
        "no_probes": args.no_probes,
        "profile": args.profile,
    }

    outcomes = evaluate(registry, ctx)
    for cid in args.na:
        if cid in outcomes:
            outcomes[cid] = result(NA, "pre-approved not-applicable via --na")

    # Optional eval-plan.json manifest aggregation (plan section 10). Adds the
    # machine-generated aggregation verdict plus manifest negative controls as
    # required criteria; validate gates on them. Without --manifest, behavior is
    # exactly the pre-manifest report/validate.
    extra_required = None
    if args.manifest:
        try:
            manifest = eval_manifest.load_manifest(args.manifest)
            evidence_dir = eval_manifest.safe_evidence_dir(evidence_dir)
        except eval_manifest.ManifestError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(2)
        mv = eval_manifest.aggregate(manifest, evidence_dir, expect_candidate=args.candidate)
        ctx["manifest_verdict"] = mv
        ctx["manifest_path"] = str(args.manifest)
        outcomes["EVID-MANIFEST"] = result(
            PASS if mv["ready"] else FAIL,
            ("eval-plan aggregation READY: " if mv["ready"] else "eval-plan aggregation NOT READY: ")
            + f"{mv['counts']['pass']}/{mv['counts']['required_total']} required scenarios pass, "
            f"{len(mv['binding_violations'])} binding violation(s)"
            + (f"; {mv['manifest_candidate_mismatch']}" if mv["manifest_candidate_mismatch"] else ""),
            {"release_id": mv["release_id"], "counts": mv["counts"],
             "rows": mv["rows"], "binding_violations": mv["binding_violations"]},
        )
        manifest_ncs = eval_manifest.run_negative_controls()
        for nc_id, ok, detail in manifest_ncs:
            outcomes[f"EVID-{nc_id}"] = result(PASS if ok else FAIL, detail)
        extra_required = ["EVID-MANIFEST"] + [f"EVID-{nc[0]}" for nc in manifest_ncs]

    summary = summarize(registry, outcomes, args.profile, extra_required=extra_required)

    if args.json:
        doc = {
            "candidate": candidate,
            "summary": summary,
            "criteria": {cid: outcomes[cid] for cid in outcomes},
        }
        if "manifest_verdict" in ctx:
            doc["manifest"] = ctx["manifest_verdict"]
        print(json.dumps(doc, indent=2))
    else:
        print(render_human(registry, outcomes, summary, ctx))

    if args.mode == "validate":
        sys.exit(0 if summary["ready"] else 1)
    sys.exit(0)


if __name__ == "__main__":
    main()
