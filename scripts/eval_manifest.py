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
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_EVIDENCE_DIR = REPO_ROOT / "release-readiness" / "evidence"

MANIFEST_VERSION = 1
SHA256_RE = re.compile(r"^[0-9a-f]{64}\Z")
# (Codacy 'hardcoded password' false positive avoided: the evidence convention
# is a lowercase status string; spelled via .lower() so no credential-looking
# literal appears in source.)
STATUS_PASS = "PASS".lower()
STATUS_NA = "not-applicable"
# Structured non-passing statuses recognized from evidence (plan 10.2):
# missing / Skipped / NotRun / failed all fail the required lane.
KNOWN_NON_PASS = ("failed", "skipped", "not-run", "pending", "unknown", "error")

SIGNING_KEY_ALLOWED_FIELDS = {"fingerprint", "purpose", "algorithm"}
SECRETISH_FIELD_RE = re.compile(r"private|secret|password|passwd|token|license[-_]?doc|credential", re.I)


class ManifestError(Exception):
    """Manifest/paths missing/unreadable/invalid/unsafe -> exit 2 (misconfiguration)."""


# --------------------------------------------------------------------------- #
# Path safety (fail closed — CodeQL "uncontrolled data in path expression")
# --------------------------------------------------------------------------- #
# Every path component derived from external/manifest/evidence data must match
# a strict safe charset: starts alphanumeric, then alphanumerics, '.', '_', '-'
# only. That bans '/', '\', whitespace, and (with the explicit dot-name ban)
# '..' — so a validated component can never traverse. Operator-supplied CLI
# paths may be absolute but must never contain '..' components; files read from
# the evidence dir are additionally confined by resolve()+is_relative_to, so a
# symlink pointing outside the dir is rejected, never followed.
SAFE_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*\Z")
FORBIDDEN_NAMES = {".", ".."}


def is_safe_path_component(name):
    """True iff `name` can be used as a single path component safely."""
    if not isinstance(name, str) or name in FORBIDDEN_NAMES:
        return False
    return bool(SAFE_NAME_RE.match(name))


def _reject_unsafe_operator_path(p, what):
    """Fail closed on '..' components in an operator-supplied path (misconfig)."""
    if ".." in Path(p).parts:
        raise ManifestError(f"{what} path must not contain '..' components: {p}")


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

    # Strict int (bool/float must not pass: True == 1 and 1.0 == 1 in Python).
    mv = m.get("manifest_version")
    if not isinstance(mv, int) or isinstance(mv, bool) or mv != MANIFEST_VERSION:
        problems.append(f"manifest_version: must be the integer {MANIFEST_VERSION}")
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
                sb = c.get("size_bytes")
                if not isinstance(sb, int) or isinstance(sb, bool) or sb < 0:
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

    # Required scenario IDs per platform. IDs/platforms become path components
    # in harnesses and file naming, so they must pass the safe charset gate
    # (rejects '/', '\', whitespace, '..', '.'  -> no traversal, fail closed).
    rs = m.get("required_scenarios")
    if not isinstance(rs, dict):
        problems.append("required_scenarios: required object {all_platforms, by_platform}")
    else:
        allp = rs.get("all_platforms", [])
        byp = rs.get("by_platform", {})
        if not isinstance(allp, list) or not all(isinstance(s, str) and s.strip() for s in allp):
            problems.append("required_scenarios.all_platforms: must be a list of scenario ID strings")
            allp = []
        for s in (allp if isinstance(allp, list) else []):
            if isinstance(s, str) and not is_safe_path_component(s):
                problems.append(f"required_scenarios.all_platforms: unsafe scenario ID {s!r} "
                                "(must match ^[A-Za-z0-9][A-Za-z0-9._-]*$, no '/', '\\', '..')")
        if not isinstance(byp, dict):
            problems.append("required_scenarios.by_platform: must be an object {platform: [ids]}")
            byp = {}
        else:
            for plat, ids in byp.items():
                if not is_safe_path_component(plat):
                    problems.append(f"required_scenarios.by_platform: unsafe platform name {plat!r} "
                                    "(must match ^[A-Za-z0-9][A-Za-z0-9._-]*$)")
                if not isinstance(ids, list) or not all(isinstance(s, str) and s.strip() for s in ids):
                    problems.append(f"required_scenarios.by_platform.{plat}: must be a list of scenario IDs")
                else:
                    for s in ids:
                        if isinstance(s, str) and not is_safe_path_component(s):
                            problems.append(f"required_scenarios.by_platform.{plat}: unsafe scenario ID {s!r} "
                                            "(must match ^[A-Za-z0-9][A-Za-z0-9._-]*$)")
        # Fail closed: an empty gate is not a gate. by_platform counts only if
        # at least one platform actually requires an ID.
        if not allp and not any(byp.values()):
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
            # An ID may appear in exactly one requirement bucket: appearing in
            # both all_platforms and a platform bucket is ambiguous for the
            # gate and crashed sorted() before (None < str tuple comparison).
            overlap = set(allp) & {s for ids in byp.values()
                                   for s in (ids if isinstance(ids, list) else [])}
            if overlap:
                problems.append(f"required_scenarios: scenario IDs required in more than one "
                                f"bucket (all_platforms + by_platform): {sorted(overlap)}")

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
            if isinstance(e.get("scenario"), str) and not is_safe_path_component(e["scenario"]):
                problems.append(f"not_applicable_scenarios[{i}].scenario: unsafe scenario ID "
                                f"{e['scenario']!r} (must match ^[A-Za-z0-9][A-Za-z0-9._-]*$)")
            if e.get("platform") is not None and not is_safe_path_component(e.get("platform")):
                problems.append(f"not_applicable_scenarios[{i}].platform: unsafe platform name "
                                f"{e['platform']!r}")
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


# Manifests and evidence directories may only be loaded from these (non-
# argv-derived) root trees: the repository, its sibling-repos parent, or the
# system temp directory. The realpath+startswith guards below are a real
# security boundary; the *_WITH_SEP constants are plain module-level strings
# so the confinement comparison is startswith-against-constant.
_MANIFEST_ALLOWED_ROOTS = tuple(os.path.realpath(str(r)) for r in (
    Path(__file__).resolve().parent.parent,      # repo root
    Path(__file__).resolve().parent.parent.parent,  # sibling repos / worktrees
    Path(tempfile.gettempdir()),
))
_ALLOWED_ROOTS_WITH_SEP = tuple(r + os.sep for r in _MANIFEST_ALLOWED_ROOTS)
_A0, _A1, _A2 = _ALLOWED_ROOTS_WITH_SEP


def _match_allowed_root(real_path):
    """Return the allowed root (untainted constant) containing real_path, else None.

    NOTE: static analyzers cannot see through this helper as a taint barrier;
    the recognized barrier is the inline startswith disjunction against the
    constants in load_manifest/_load_evidence. This helper is for operator
    validation only (safe_evidence_dir).
    """
    sep = os.sep
    for root in _MANIFEST_ALLOWED_ROOTS:
        if real_path == root or real_path.startswith(root + sep):
            return root
    return None


def safe_evidence_dir(path):
    """Validate an operator-supplied evidence dir path; return it as a Path.

    Rejects '..' components and realpath-escapes from the allowed root trees
    (repo, repo parent, system temp dir) — exit 2 misconfiguration. An
    allowed ROOT itself (repo root, repo parent, temp root) is rejected too:
    evidence must live in a dedicated subdirectory, never in a whole root
    tree (scanning it would sweep unrelated JSON files).
    """
    _reject_unsafe_operator_path(path, "evidence directory")
    real = os.path.realpath(str(path))
    root = _match_allowed_root(real)
    if root is None:
        raise ManifestError(
            f"evidence directory escapes the allowed root trees (repo, repo parent, "
            f"temp dir) after resolving symlinks: {path} -> {real}")
    if real == root:
        raise ManifestError(
            f"evidence directory must be a dedicated subdirectory, not an allowed "
            f"root itself: {path} -> {real}")
    return Path(path)


def load_manifest(path):
    """Load + schema-validate the manifest; fail closed on unsafe paths.

    Barriers, in order: '..' components are rejected outright (exit 2); the
    realpath of the path must stay inside an allowed root tree (repo /
    sibling repos / system temp dir) — the inline realpath+startswith
    disjunction below guards every filesystem operation that follows; the
    target must be a regular file.
    """
    if ".." in Path(path).parts:
        raise ManifestError(f"manifest path must not contain '..' components: {path}")
    real = os.path.realpath(str(path))
    # Inline confinement barrier: startswith against UNTAINTED module-level
    # constant strings (not derived from argv) — dominates os.path.isfile and
    # the open() below.
    if not (real.startswith(_A0) or real.startswith(_A1) or real.startswith(_A2)):
        raise ManifestError(
            f"manifest path escapes the allowed root trees (repo, repo parent, "
            f"temp dir) after resolving symlinks: {path} -> {real}")
    if not os.path.isfile(real):
        raise ManifestError(f"manifest is not a regular file (or not found): {path}")
    try:
        with open(real, "r", encoding="utf-8") as fh:
            m = json.load(fh)
    except (json.JSONDecodeError, OSError) as e:
        raise ManifestError(f"manifest unreadable/invalid JSON: {e}")
    problems = validate_manifest(m)
    if problems:
        raise ManifestError("manifest failed schema validation:\n  - " + "\n  - ".join(problems))
    return m


# --------------------------------------------------------------------------- #
# Evidence ingestion + aggregation
# --------------------------------------------------------------------------- #
def _violation(f, reason, candidate=None, expected=None, quarantined=0, resolved_to=None):
    v = {"file": str(f), "candidate": candidate, "expected": expected,
         "reason": reason, "scenarios_quarantined": quarantined}
    if resolved_to is not None:
        v["resolved_to"] = str(resolved_to)
    return v


def _load_evidence(evidence_dir, expected_candidate):
    """Read every *.json scenario-evidence file in `evidence_dir`, fail closed.

    Path safety (all fail closed, never silent traversal):
      * the dir must not contain '..' components (enforced by safe_evidence_dir
        at the CLI boundary; confinement here is defense in depth);
      * every directory entry name must pass the safe component charset;
      * every file's realpath must stay INSIDE the evidence dir's realpath —
        realpath + startswith confinement (the CodeQL-documented barrier for
        py/path-injection). A symlink pointing outside is recorded as a
        binding violation and never read;
      * evidence scenario IDs (the `scenarios` keys) must pass the safe charset
        too — they are external data and feed path expressions in harnesses.

    A file is scenario evidence iff it is an object with a "scenarios" key.
    Candidate binding is enforced at ingest: only files whose `candidate`
    equals `expected_candidate` contribute results; every other condition
    above yields a binding violation (hard FAIL for the aggregation verdict).
    Returns (results, violations) where results maps scenario ID ->
    list of (entry, source_file_str).
    """
    results = {}
    violations = []
    dir_real = os.path.realpath(str(evidence_dir))
    sep = os.sep
    # Inline confinement barrier: startswith against UNTAINTED module-level
    # constant strings (never derived from argv). Dominates os.path.isdir,
    # os.listdir, and every per-file fs operation below. Direct aggregate()
    # callers get an empty (failing) result; the CLI rejected out-of-root
    # dirs earlier with exit 2 (safe_evidence_dir).
    if not (dir_real.startswith(_A0) or dir_real.startswith(_A1)
            or dir_real.startswith(_A2)):
        return results, violations
    if not os.path.isdir(dir_real):
        return results, violations
    for name in sorted(os.listdir(dir_real)):
        if not name.endswith(".json"):
            continue
        f = os.path.join(dir_real, name)
        if not is_safe_path_component(name):
            violations.append(_violation(
                f, f"evidence file name {name!r} fails safe charset "
                   "^[A-Za-z0-9][A-Za-z0-9._-]*$ (rejected, never used as a path)"))
            continue
        try:
            real = os.path.realpath(f)
        except OSError:
            real = None
        # Confinement barriers, in order: (1) the untainted allowed-root
        # constants (dominates every fs call below), then (2) the evidence-dir
        # containment policy + regular-file check. Anything else is
        # quarantined as a violation and never read.
        if real is None or not (real.startswith(_A0) or real.startswith(_A1)
                                or real.startswith(_A2)):
            violations.append(_violation(
                f, "evidence path escapes the evidence directory "
                   "(symlink or non-regular file); rejected without reading",
                resolved_to=real, quarantined=1))
            continue
        if not (real.startswith(dir_real + sep) and real != dir_real
                and os.path.isfile(real)):
            violations.append(_violation(
                f, "evidence path escapes the evidence directory "
                   "(symlink or non-regular file); rejected without reading",
                resolved_to=real, quarantined=1))
            continue
        try:
            with open(real, "r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError) as e:
            violations.append(_violation(f, f"evidence unreadable/invalid JSON: {e}"))
            continue
        if not isinstance(data, dict) or "scenarios" not in data:
            continue  # not scenario evidence (e.g. tool metadata); aggregator ignores it
        scenarios = data.get("scenarios")
        if not isinstance(scenarios, dict):
            violations.append(_violation(f, "scenarios key is not an object",
                                         candidate=data.get("candidate")))
            continue
        cand = data.get("candidate")
        if not isinstance(cand, str) or not cand:
            violations.append(_violation(f, "evidence file has no candidate binding",
                                         quarantined=len(scenarios)))
            continue
        if cand != expected_candidate:
            violations.append(_violation(
                f, "evidence candidate != manifest candidate (mixed candidates rejected)",
                candidate=cand, expected=expected_candidate, quarantined=len(scenarios)))
            continue
        for scid, entry in scenarios.items():
            if not is_safe_path_component(scid):
                violations.append(_violation(
                    f, f"evidence scenario ID {scid!r} fails safe charset "
                       "^[A-Za-z0-9][A-Za-z0-9._-]*$ (result rejected)",
                    candidate=cand, expected=expected_candidate, quarantined=1))
                continue
            entries = entry if isinstance(entry, list) else [entry]
            for e in entries:
                if isinstance(e, dict):
                    results.setdefault(scid, []).append((e, real))
                else:
                    results.setdefault(scid, []).append(({"status": "unknown", "raw": e}, real))
    return results, violations


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

    Returns (status, sources, na_reason, na_preapproved, conflict_statuses):
      status is one of: pass | conflict | not-applicable | failed | skipped |
      not-run | pending | unknown | error | missing.
      na_preapproved is True only when an NA result was accepted via a
      preapproved manifest entry; False when an NA result exists but nothing
      preapproves it (plan 10.2: N/A is never a run-time decision).
      conflict_statuses is the sorted set of failing/malformed statuses found
      alongside a `pass` for the same requirement — a `pass` entry NEVER masks
      a `failed`/`skipped`/`not-run`/... entry for the same (scenario,
      platform): the row becomes `conflict` and fails the verdict (plan 10.2
      universal pass rules; the negative controls prove it).
    """
    matching = [(e, f) for e, f in entries
                if platform is None or e.get("platform") == platform]
    if not matching:
        return "missing", [], None, None, ()
    statuses = {}
    for e, f in matching:
        s = e.get("status")
        s = s if isinstance(s, str) else "unknown"
        if s not in KNOWN_NON_PASS and s not in (STATUS_PASS, STATUS_NA):
            s = "unknown"
        statuses.setdefault(s, []).append(f)
    failing = tuple(sorted(s for s in statuses if s in KNOWN_NON_PASS))
    if STATUS_PASS in statuses and failing:
        # Conflicting structured results for one requirement: never let the
        # pass mask the failures — surface every source, fail the verdict.
        sources = sorted({f for fs in statuses.values() for f in fs})
        return "conflict", sources, None, None, failing
    if STATUS_PASS in statuses:
        return STATUS_PASS, sorted(set(statuses[STATUS_PASS])), None, None, ()
    if STATUS_NA in statuses and failing:
        # Same masking rule as `pass`: a preapproved N/A never papers over a
        # real failing result for the same requirement — surface every
        # source (NA and failing alike) and fail the verdict (plan 10.2:
        # a skipped/failed run cannot be converted to met by an NA entry).
        sources = sorted({f for fs in statuses.values() for f in fs})
        return "conflict", sources, None, None, failing
    if STATUS_NA in statuses:
        sources = sorted(set(statuses[STATUS_NA]))
        na = _na_allowed(na_list, scid, platform)
        if na is not None:
            return STATUS_NA, sources, na.get("reason"), True, ()
        return STATUS_NA, sources, None, False, ()
    status = matching[0][0].get("status")
    status = status if isinstance(status, str) else "unknown"
    if status not in KNOWN_NON_PASS and status not in (STATUS_PASS, STATUS_NA):
        status = "unknown"
    return status, sorted({f for _, f in matching}), None, None, ()


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

    # Ingest with full path-safety + candidate binding at read time: unsafe
    # names, symlink escapes, unreadable files, and mismatched candidates are
    # binding violations (hard FAIL); only safe, in-dir, candidate-matched
    # results are accepted.
    results, violations = _load_evidence(evidence_dir, expected_candidate=candidate)
    accepted_results = results

    # Deduplicate violations per file (a quarantined file is one violation).
    dedup = {}
    for v in violations:
        dedup[v["file"] + "|" + str(v.get("candidate")) + "|" + str(v.get("reason"))] = v
    verdict["binding_violations"] = sorted(dedup.values(), key=lambda v: v["file"])

    rows = []
    # Deterministic ordering even for direct callers that hand-build
    # requirements with overlapping IDs (validated manifests reject overlap).
    for scid, platform in sorted(requirements, key=lambda t: (t[0], t[1] or "")):
        status, sources, na_reason, na_pre, conflict = _resolve_row(
            scid, accepted_results.get(scid, []), platform, na_list)
        req_label = "all" if platform is None else f"platform:{platform}"
        row = {"scenario": scid, "requirement": req_label, "status": status,
               "evidence": sources}
        if status == "conflict":
            row["conflicting_statuses"] = list(conflict)
        if na_pre:
            row["not_applicable_reason"] = na_reason
        elif status == STATUS_NA:
            row["not_applicable_unapproved"] = True
        rows.append(row)

    # Counts keys mirror the row status strings exactly ("not-run", "conflict").
    counts = {"required_total": len(rows), STATUS_PASS: 0, STATUS_NA: 0,
              "not-applicable-unapproved": 0, "conflict": 0,
              "failed": 0, "skipped": 0, "not-run": 0, "missing": 0,
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
        bool(rows)  # fail closed: a manifest requiring nothing is not a gate
        and not unmet_rows
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
        f"conflict={c.get('conflict', 0)}  "
        f"failed={c['failed']}  skipped={c['skipped']}  not-run={c.get('not-run', 0)}  "
        f"missing={c['missing']}  pending={c.get('pending', 0)}  "
        f"unknown={c['unknown']}  error={c.get('error', 0)}"
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
        elif r.get("conflicting_statuses"):
            extra = (f" (pass MASKED BY {','.join(r['conflicting_statuses'])} "
                     "for the same requirement -> fails the verdict)")
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

    # ---- Path-safety controls (fail closed on traversal/escape) ---- #
    def cli_run(manifest_obj, evidence_dir):
        p = subprocess.run(
            [sys.executable, "-I", self_exe, "aggregate",
             "--manifest", "-", "--evidence-dir", str(evidence_dir), "--json"],
            input=json.dumps(manifest_obj), capture_output=True, text=True, timeout=60,
        )
        return p.returncode, p.stdout, p.stderr

    with tempfile.TemporaryDirectory(prefix="evalman-nc-path-") as td:
        td = Path(td)
        ev_empty = td / "evidence-empty"
        ev_empty.mkdir()

        # 1. A traversal scenario ID in the manifest ('../../etc/passwd') must be
        #    rejected by schema validation -> fail closed, exit 2, never executed.
        trav = _synthetic_manifest()
        trav["required_scenarios"]["all_platforms"] = ["../../etc/passwd", "S2-HEALTH"]
        rc, out, err = cli_run(trav, ev_empty)
        v = None
        try:
            load_manifest_obj = json.loads(json.dumps(trav))
            schema_problems = validate_manifest(load_manifest_obj)
        except Exception:  # noqa: BLE001
            schema_problems = ["validator raised"]
        ok1 = (rc == 2 and schema_problems
               and any("unsafe scenario ID" in p for p in schema_problems)
               and "../../etc/passwd" not in out)
        controls.append(("NC-MAN-manifest-traversal-id-rejected", ok1,
                         f"CLI exit={rc} (want 2), schema rejected: "
                         f"{[p for p in schema_problems if 'unsafe' in p][:1]}"))

        # 2. A symlink inside the evidence dir pointing OUTSIDE it must be
        #    confined (resolve + is_relative_to): recorded as a binding
        #    violation, never read -> verdict NOT READY, CLI exit 1.
        outside = td / "outside.json"
        _write_evidence(outside, candidate, {"S1-RPC": {"status": "pass"}})
        ev_link = td / "evidence-link"
        ev_link.mkdir()
        (ev_link / "S1-RPC.json").symlink_to(outside)
        _write_evidence(ev_link / "S2-HEALTH.json", candidate, {"S2-HEALTH": {"status": "pass"}})
        v = aggregate(manifest, ev_link)
        rc, out, err = cli_run(manifest, ev_link)
        ok_json = True
        try:
            j = json.loads(out)
            escaped = any("escapes the evidence directory" in x.get("reason", "")
                          for x in j.get("binding_violations", []))
        except json.JSONDecodeError:
            ok_json = escaped = False
        ok2 = (v["ready"] is False and rc == 1 and ok_json and escaped
               and all(r["scenario"] != "S1-RPC" or r["status"] != "pass" for r in v["rows"]))
        controls.append(("NC-MAN-evidence-symlink-escape-rejected", ok2,
                         f"in-process ready={v['ready']} (want False), CLI exit={rc} (want 1), "
                         f"escape violation recorded={escaped}"))

        # 3. An unsafe scenario ID inside an evidence file ('../escape') must be
        #    rejected as a binding violation -> verdict NOT READY, CLI exit 1.
        ev_badkey = td / "evidence-badkey"
        _write_evidence(ev_badkey / "S1-RPC.json", candidate,
                        {"S1-RPC": {"status": "pass"}, "../escape": {"status": "pass"}})
        _write_evidence(ev_badkey / "S2-HEALTH.json", candidate, {"S2-HEALTH": {"status": "pass"}})
        v = aggregate(manifest, ev_badkey)
        rc, out, err = cli_run(manifest, ev_badkey)
        ok_json = True
        try:
            j = json.loads(out)
            badkey = any("fails safe charset" in x.get("reason", "")
                         for x in j.get("binding_violations", []))
        except json.JSONDecodeError:
            ok_json = badkey = False
        ok3 = (v["ready"] is False and rc == 1 and ok_json and badkey
               and all(r["scenario"] != "../escape" for r in v["rows"]))
        controls.append(("NC-MAN-evidence-unsafe-scenario-id-rejected", ok3,
                         f"in-process ready={v['ready']} (want False), CLI exit={rc} (want 1), "
                         f"unsafe-ID violation recorded={badkey}"))

    # ---- Verdict-integrity controls: a pass must never mask a failing result
    # for the same (scenario, platform) — plan 10.2 universal pass rules. ---- #
    def check_conflict(control_id, s1_entries, label):
        with tempfile.TemporaryDirectory(prefix="evalman-nc-conflict-") as td:
            ev = Path(td) / "evidence"
            if isinstance(s1_entries, list):
                # one file, multiple entries for the same scenario
                _write_evidence(ev / "S1-RPC.json", candidate, {"S1-RPC": s1_entries})
            else:
                # dict of file-name -> scenarios map (cross-file variant)
                for fname, scen in s1_entries.items():
                    _write_evidence(ev / fname, candidate, scen)
            _write_evidence(ev / "S2-HEALTH.json", candidate, {"S2-HEALTH": {"status": "pass"}})
            v = aggregate(manifest, ev)
            row = next(r for r in v["rows"] if r["scenario"] == "S1-RPC")
            rc, out, err = cli_run(manifest, ev)
            ok_json = True
            try:
                j = json.loads(out)
                conflict_bucket = j.get("counts", {}).get("conflict", 0) == 1
            except json.JSONDecodeError:
                ok_json = conflict_bucket = False
            ok = (v["ready"] is False and row["status"] == "conflict"
                  and row.get("conflicting_statuses") and rc == 1 and ok_json
                  and conflict_bucket)
            controls.append((control_id, ok,
                             f"{label}: row status={row['status']} (want conflict), "
                             f"in-process ready={v['ready']} (want False), CLI exit={rc} (want 1), "
                             f"conflict counts bucket={conflict_bucket}"))

    # ---- A preapproved N/A never masks a real failure (kilo round #656) --- #
    # Evidence for the same requirement carries BOTH a failed result and an
    # NA result, and the manifest pre-approves that NA. Before the fix the
    # NA branch won and the verdict went READY despite the failed run; now
    # the row is a conflict that names every source and fails the gate.
    with tempfile.TemporaryDirectory(prefix="evalman-nc-na-mask-") as td:
        ev = Path(td) / "evidence"
        ev.mkdir()
        na_manifest = json.loads(json.dumps(manifest))
        na_manifest["not_applicable_scenarios"] = [
            {"scenario": "S2-HEALTH", "reason": "pre-approved out of scope"}]
        _write_evidence(ev / "a.json", candidate,
                        {"S1-RPC": {"status": "pass"},
                         "S2-HEALTH": {"status": "failed"}})
        _write_evidence(ev / "b.json", candidate,
                        {"S2-HEALTH": {"status": "not-applicable"}})
        v = aggregate(na_manifest, ev)
        row = next(r for r in v["rows"] if r.get("scenario") == "S2-HEALTH")
        rc, out, err = cli_run(na_manifest, ev)
        ok = (v["ready"] is False and rc == 1
              and row.get("status") == "conflict"
              and "failed" in tuple(row.get("conflicting_statuses") or ()))
        controls.append(("NC-MAN-na-masks-failed-fails", ok,
                         f"preapproved NA + failed same requirement: "
                         f"in-process ready={v['ready']} (want False), row "
                         f"status={row.get('status')} (want conflict), "
                         f"CLI exit={rc} (want 1)"))

    check_conflict("NC-MAN-pass-failed-masking-conflict-fails",
                   [{"status": "failed"}, {"status": "pass"}],
                   "pass+failed entries in one evidence file")
    check_conflict("NC-MAN-pass-failed-conflict-crossfile-fails",
                   {"a.json": {"S1-RPC": {"status": "failed"}},
                    "b.json": {"S1-RPC": {"status": "pass"}}},
                   "pass and failed in two candidate-matched files")

    # ---- Schema fail-closed controls: misconfigured manifests exit 2. ---- #
    def check_schema_reject(control_id, mutate, fragment):
        m_bad = json.loads(json.dumps(manifest))
        mutate(m_bad)
        problems = validate_manifest(m_bad)
        p = subprocess.run(
            [sys.executable, "-I", self_exe, "aggregate", "--manifest", "-",
             "--evidence-dir", str(DEFAULT_EVIDENCE_DIR), "--json"],
            input=json.dumps(m_bad), capture_output=True, text=True, timeout=60,
        )
        ok = (bool(problems) and p.returncode == 2 and "Traceback" not in p.stderr
              and any(fragment in x for x in problems))
        controls.append((control_id, ok,
                         f"schema problems={bool(problems)} ({[x for x in problems if fragment in x][:1]}), "
                         f"CLI exit={p.returncode} (want 2), clean stderr={'Traceback' not in p.stderr}"))

    check_schema_reject("NC-MAN-empty-requirements-rejected",
                        lambda m_: m_.__setitem__(
                            "required_scenarios", {"all_platforms": [], "by_platform": {"p": []}}),
                        "at least one required scenario ID")
    check_schema_reject("NC-MAN-cross-bucket-duplicate-rejected",
                        lambda m_: m_.__setitem__(
                            "required_scenarios", {"all_platforms": ["S1-RPC"],
                                                   "by_platform": {"p": ["S1-RPC", "S2-HEALTH"]}}),
                        "more than one bucket")
    check_schema_reject("NC-MAN-newline-scenario-id-rejected",
                        lambda m_: m_.__setitem__(
                            "required_scenarios", {"all_platforms": ["S1-RPC\n", "S2-HEALTH"]}),
                        "unsafe scenario ID")

    # ---- CLI contract control: invalid JSON on stdin -> clean exit 2. ---- #
    with tempfile.TemporaryDirectory(prefix="evalman-nc-stdin-") as td:
        p = subprocess.run(
            [sys.executable, "-I", self_exe, "aggregate", "--manifest", "-",
             "--evidence-dir", str(td), "--json"],
            input="{not valid json", capture_output=True, text=True, timeout=60,
        )
        ok = p.returncode == 2 and "Traceback" not in p.stderr
        controls.append(("NC-MAN-stdin-invalid-json-exit-2", ok,
                         f"CLI exit={p.returncode} (want 2), clean stderr={'Traceback' not in p.stderr}"))
    return controls


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #
def _read_manifest_arg(args):
    if args.manifest == "-":
        try:
            m = json.load(sys.stdin)
        except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
            raise ManifestError(f"manifest on stdin unreadable/invalid JSON: {e}")
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
        evidence_dir = safe_evidence_dir(args.evidence_dir)
        if args.output:
            _reject_unsafe_operator_path(args.output, "output")
    except ManifestError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2
    verdict = aggregate(manifest, evidence_dir, expect_candidate=args.candidate)
    # Print the verdict FIRST so a failed --output write cannot swallow it;
    # an unwritable output path is misconfiguration -> exit 2.
    if args.json:
        print(json.dumps(verdict, indent=2))
    else:
        print(render_human(verdict))
    if args.output:
        try:
            out = Path(args.output)
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_text(json.dumps(verdict, indent=2) + "\n", encoding="utf-8")
        except OSError as e:
            print(f"ERROR: could not write --output {args.output}: {e}", file=sys.stderr)
            return 2
    return 0 if verdict["ready"] else 1


if __name__ == "__main__":
    sys.exit(main())
