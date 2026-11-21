#!/usr/bin/env python3
"""Release-coverage gate for OdbDesign (criterion M0-01 / SRC-01).

Parses RAW LCOV tracefiles and RAW JUnit XML — never human summary text —
and enforces:

  * Strict integer thresholds on the HANDWRITTEN denominator:
      - lines     : 100 * lh > min_line * lf   (strict >90%: exactly 90.00% FAILS)
      - functions : 100 * fnh > min_func * fnf (strict >90%: exactly 90.00% FAILS)
      - branches  : 100 * brh >= min_branch * brf  (>=85% per SRC-01)
    Integer arithmetic only: no float rounding can turn an exact-boundary
    denominator into a pass.
  * Branch data presence: a handwritten tracefile with zero branch-found
    (BRF) counters is a finding ("missing branch data"), not a pass.
  * Inventory completeness: every required production TU must appear as an
    SF record in the handwritten tracefile (previously-omitted TUs must be
    present with real or zero-hit counts — absent is a finding).
  * Handwritten + generated reconciliation: when all three tracefiles are
    given, per-metric sums must add up to the combined tracefile exactly,
    so the scopes can never silently disagree with the audit report.
  * Test outcomes from JUnit: any failed/error test is a finding; any
    skipped test is a finding unless --allow-skips; tests below
    --expected-min-tests (when given) are a finding.

Usage:
    check_coverage.py --handwritten coverage.info [--generated gen.info]
        [--combined comb.info] --junit results.xml
        [--min-line 90 --min-func 90 --min-branch 85]
        [--require-inventory TU.cpp ...] [--expected-min-tests N]
        [--allow-skips]

Exit codes (repo convention):
    0  pass
    1  finding (gate violated)
    2  misconfiguration (missing/unreadable input, malformed file, bad args)

Function counts are consumed from the tracefile's own FNF/FNH rollups — the
same integers recorded by lcov 2.0 (CI) and read by
scripts/release_readiness.py — never from human-readable summaries. (lcov
>= 2.5's HTML/summary function rates may additionally deduplicate function
aliases; the recorded rollups remain the authoritative, version-stable
denominator.)

The JUnit XML is parsed with stdlib html.parser (NOT xml.*): the gate's
inputs are treated as untrusted, and html.parser's strict-mode reader here
rejects DOCTYPE/entity declarations outright (the entity-expansion vector
that the use-defused-xml security rule exists for) while validating tag
nesting. Standard library only, so the gate runs on machines without
third-party packages.

Run with: python3 -I scripts/check_coverage.py ...
"""

from __future__ import annotations

import argparse
import json
import sys
from html.parser import HTMLParser
from pathlib import Path

PASS, FINDING, MISCONFIG = 0, 1, 2

# The nine substantive production TUs previously omitted from the coverage
# denominator (release_readiness_criteria.json M0-01). Used when
# --require-inventory is not passed explicitly.
DEFAULT_INVENTORY = [
    "FileModelController.cpp",
    "DesignsController.cpp",
    "FileUploadController.cpp",
    "HealthCheckController.cpp",
    "HelloWorldController.cpp",
    "OdbDesignServerApp.cpp",
    "OdbServerAppBase.cpp",
    "main.cpp",
    "OdbDesignApp.cpp",
]

METRICS = ("lf", "lh", "fnf", "fnh", "brf", "brh")
# LCOV tracefile rollup record keys (uppercase in the tracefile) mapped to
# the internal lowercase metric names. Detail records (SF/DA/FN/FNDA/BRDA)
# are not rollups and are ignored.
TRACE_KEYS = {"LF": "lf", "LH": "lh", "FNF": "fnf", "FNH": "fnh",
              "BRF": "brf", "BRH": "brh"}


def parse_lcov(path: Path) -> dict[str, dict[str, int]]:
    """Parse an LCOV tracefile into per-SF-record integer counters.

    Only integer arithmetic is used anywhere downstream. Raises ValueError
    on structurally malformed records.
    """
    records: dict[str, dict[str, int]] = {}
    current: dict[str, int] | None = None
    current_sf = "<none>"
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for lineno, raw in enumerate(f, 1):
            line = raw.rstrip("\n")
            if not line:
                continue
            if line.startswith("SF:"):
                sf = line[3:]
                if not sf:
                    raise ValueError(f"{path}:{lineno}: empty SF record")
                if sf in records:
                    raise ValueError(f"{path}:{lineno}: duplicate SF record: {sf}")
                current = {m: 0 for m in METRICS}
                current_sf = sf
                records[sf] = current
            elif line.startswith("end_of_record"):
                if current is None:
                    raise ValueError(f"{path}:{lineno}: end_of_record without SF")
                current = None
            elif current is not None:
                key, sep, value = line.partition(":")
                if sep and key in TRACE_KEYS:
                    metric = TRACE_KEYS[key]
                    try:
                        current[metric] = int(value)
                    except ValueError as e:
                        raise ValueError(
                            f"{path}:{lineno}: non-integer {key}: {value!r}"
                        ) from e
                # FN/DA/BRDA detail lines are ignored; the rollups LF/LH/
                # FNF/FNH/BRF/BRH (which geninfo guarantees to emit) are the
                # authoritative integer counts this gate consumes.
    if current is not None:
        raise ValueError(f"{path}: truncated record (no end_of_record): {current_sf}")
    if not records:
        raise ValueError(f"{path}: no SF records found")
    return records


def totals(records: dict[str, dict[str, int]]) -> dict[str, int]:
    """Sum per-record integer counters into file-level totals."""
    out = {m: 0 for m in METRICS}
    for rec in records.values():
        for m in METRICS:
            out[m] += rec[m]
    return out


def strict_gt(hit: int, found: int, min_pct: int) -> bool:
    """Strict 'hit rate > min_pct%': exactly equal FAILS."""
    return 100 * hit > min_pct * found


def nonstrict_ge(hit: int, found: int, min_pct: int) -> bool:
    """>=min_pct%."""
    return 100 * hit >= min_pct * found


def pct_str(hit: int, found: int) -> str:
    """Format a hit/found pair as a percentage string (n/a when empty)."""
    return "n/a" if found == 0 else f"{(100 * hit) / found:.2f}% ({hit}/{found})"


class _JUnitReader(HTMLParser):
    """Event reader for JUnit XML that fails closed on XML attacks.

    html.parser is used instead of xml.etree because this gate's inputs are
    treated as UNTRUSTED (the rule behind Codacy/opengrep use-defused-xml):
    entity-expansion attacks enter through DTD declarations, so any
    <!DOCTYPE>/<!ENTITY> declaration raises instead of being parsed.
    HTMLParser alone is lenient, so tag nesting is validated by hand
    (mismatched or unclosed tags are errors), preserving the strictness the
    previous xml.etree path had.
    """

    def __init__(self, path: Path) -> None:
        super().__init__(convert_charrefs=True)
        self._path = path
        self.root_tag: str | None = None
        self.root_attrs: dict[str, str] = {}
        self.suite_attrs: list[dict[str, str]] = []
        self.outcome_elements = {"skipped": 0, "failure": 0, "error": 0}
        self.disabled_total = 0
        self.notrun_count = 0
        self._stack: list[str] = []
        self._root_closed = False

    # -- hardening: DTD / entity / declaration vectors are rejected --------
    # (billion-laughs entity expansion enters via DOCTYPE/ENTITY, and
    # html.parser routes several <! ... > spellings to lenient handlers
    # where an attacker-controlled internal subset could slip through).
    # This reader therefore rejects EVERY declaration, comment and marked
    # section unconditionally — including CDATA, whose _markupbase
    # terminator (]\s*]\s*>) tolerates '] ] >' inside the payload and would
    # end the section early, mis-parsing the remainder as markup.
    # ctest's --output-junit emits none of these, so rejection is both safe
    # for this gate and strictly safer than lenient parsing.
    def handle_decl(self, decl: str) -> None:
        """Reject DOCTYPE declarations (entity-expansion vector)."""
        raise ValueError(
            f"{self._path}: rejected DOCTYPE declaration "
            f"(entity-expansion vector not allowed): <!{decl[:80]}>")

    def unknown_decl(self, data: str) -> None:
        """Reject all marked sections, CDATA included (fail closed)."""
        raise ValueError(
            f"{self._path}: rejected marked section <![{data[:80]}>")

    def handle_comment(self, data: str) -> None:
        """Reject every comment/decl-like bogus comment (fail closed).

        html.parser routes <!DOCTYPE ...>, <!ENTITY ...>, '<! DOCTYPE ...'
        and any other <! ... > spelling that is not a real declaration to
        bogus comments here; a real XML comment never appears in
        ctest-generated JUnit. Enumerating attack spellings is fragile, so
        the gate rejects the whole class.
        """
        raise ValueError(
            f"{self._path}: rejected comment/declaration <!{data[:80]}>")

    def error(self, message: str) -> None:
        """Route _markupbase parse errors through the MISCONFIG contract.

        ParserBase.parse_marked_section calls self.error() for keywords
        outside {temp, cdata, ignore, include, rcdata, if, else, endif}
        (e.g. <![FOO[..]]>); the base implementation raises
        NotImplementedError, which escaped this checker's documented
        exit-2-for-malformed-input behavior as a raw exit-1 traceback.
        Raising ValueError here sends every parser rejection through the
        same MISCONFIG path.
        """
        raise ValueError(f"{self._path}: malformed JUnit XML: {message}")

    # -- structure ---------------------------------------------------------
    # The JUnit vocabulary this gate accepts. html.parser lowercases tag
    # names before dispatch (the original case is unrecoverable), so
    # element-name strictness is restored by rejecting anything outside
    # the standard JUnit element set instead of by case sensitivity.
    _KNOWN_TAGS = frozenset({
        "testsuites", "testsuite", "testcase", "skipped", "failure",
        "error", "properties", "property", "system-out", "system-err",
    })

    def _start(self, tag: str, attrs: dict[str, str]) -> None:
        """Record root/suite attributes and testcase outcome elements."""
        if self._root_closed:
            raise ValueError(
                f"{self._path}: markup after the document root element "
                f"closed (truncated or duplicated document?): <{tag}>")
        if tag not in self._KNOWN_TAGS:
            raise ValueError(
                f"{self._path}: unexpected element <{tag}>; not in the "
                "JUnit vocabulary this gate accepts")
        if self.root_tag is None:
            if tag not in ("testsuites", "testsuite"):
                raise ValueError(
                    f"{self._path}: unexpected root element <{tag}>; "
                    "want testsuites/testsuite")
            self.root_tag = tag
            self.root_attrs = attrs
        self._stack.append(tag)
        if tag == "testsuite":
            # Direct children of a <testsuites> root only (depth 2), or a
            # <testsuite> root itself (depth 1) — mirrors the previous
            # root.findall("testsuite") semantics.
            if len(self._stack) <= 2:
                self.suite_attrs.append(attrs)
                # gtest producers report deliberately-disabled tests via a
                # suite-level `disabled` attribute; they did not run, which
                # is the same gate concern as a skip (fail-safe direction).
                raw_disabled = attrs.get("disabled")
                if raw_disabled is not None:
                    try:
                        self.disabled_total += int(raw_disabled)
                    except ValueError as e:
                        raise ValueError(
                            f"{self._path}: non-integer disabled={raw_disabled!r}") from e
        if tag == "testcase" and attrs.get("status") == "notrun":
            # Older ctest JUnit writers may mark skips ONLY via
            # status="notrun" (no <skipped> element, no skipped= attribute);
            # counting them keeps the unexpected-skips finding fail-safe
            # across the declared ctest support range. Double counting is
            # avoided by the max() merge in parse_junit.
            self.notrun_count += 1
        if (tag in ("skipped", "failure", "error")
                and len(self._stack) >= 2
                and self._stack[-2] == "testcase"):
            # Outcome elements are counted per testcase child (same
            # semantics as the previous case.find(...) lookups).
            self.outcome_elements[tag] += 1

    def _checked_attrs(self, tag: str, attrs) -> dict[str, str]:
        """dict() the attribute list, rejecting duplicates.

        A plain dict() keeps the LAST value of a repeated attribute; the
        previous xml.etree path raised on duplicates, so the gate rejects
        them instead of silently changing semantics.
        """
        names = [name for name, _ in attrs]
        if len(names) != len(set(names)):
            dupes = sorted({n for n in names if names.count(n) > 1})
            raise ValueError(
                f"{self._path}: duplicate attribute(s) {dupes} on <{tag}>")
        return dict(attrs)

    def handle_starttag(self, tag: str, attrs) -> None:
        """Start-tag event: record and push onto the nesting stack."""
        self._start(tag, self._checked_attrs(tag, attrs))

    def handle_startendtag(self, tag: str, attrs) -> None:
        """Self-closing element (<skipped/>): start+end in one event."""
        checked = self._checked_attrs(tag, attrs)
        self._start(tag, checked)
        self._end(tag)

    def _end(self, tag: str) -> None:
        """Pop the nesting stack, rejecting mismatched end tags."""
        if not self._stack or self._stack[-1] != tag:
            raise ValueError(
                f"{self._path}: malformed JUnit XML: unexpected </{tag}>, "
                f"open elements: {self._stack}")
        self._stack.pop()
        if self.root_tag is not None and not self._stack:
            self._root_closed = True

    def handle_endtag(self, tag: str) -> None:
        """End-tag event: pop and validate."""
        self._end(tag)

    def finish(self) -> None:
        """Final structural validation after the stream ends."""
        if self.root_tag is None:
            raise ValueError(f"{self._path}: no XML root element found")
        if self._stack:
            raise ValueError(
                f"{self._path}: malformed JUnit XML: unclosed elements: "
                f"{self._stack}")


def parse_junit(path: Path) -> dict[str, int]:
    """Parse JUnit XML into integer outcome totals (tests/failures/errors/skipped)."""
    reader = _JUnitReader(path)
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for chunk in f:
            reader.feed(chunk)
    reader.close()
    reader.finish()

    counts = {"tests": 0, "failures": 0, "errors": 0, "skipped": 0}

    def add(attrs: dict[str, str]) -> None:
        """Accumulate integer outcome attributes into counts."""
        for key in counts:
            raw = attrs.get(key)
            if raw is not None:
                try:
                    counts[key] += int(raw)
                except ValueError as e:
                    raise ValueError(f"{path}: non-integer {key}={raw!r}") from e

    if reader.root_tag == "testsuites" and not reader.suite_attrs:
        # <testsuites> with no <testsuite> children: attributes live on the root.
        add(reader.root_attrs)
    else:
        for attrs in reader.suite_attrs:
            add(attrs)

    # Element counts are authoritative when suite attributes under-report.
    counts["skipped"] = max(counts["skipped"], reader.outcome_elements["skipped"])
    counts["failures"] = max(counts["failures"], reader.outcome_elements["failure"])
    counts["errors"] = max(counts["errors"], reader.outcome_elements["error"])
    # "Did not run" signals beyond <skipped>: gtest `disabled` attributes and
    # older-ctest status="notrun" testcases. Each is an under-approximation
    # of tests that did not run as expected, so the max of all signals is the
    # best estimate and cannot double-count one testcase. Fail-safe: if an
    # older producer reports skips ONLY this way, the gate still finds them.
    counts["skipped"] = max(
        counts["skipped"], reader.disabled_total, reader.notrun_count)
    return counts


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="OdbDesign release-coverage gate (raw LCOV + JUnit).")
    ap.add_argument("--handwritten", required=True, type=Path,
                    help="handwritten LCOV tracefile (the gate denominator)")
    ap.add_argument("--generated", type=Path,
                    help="generated (protobuf/gRPC) LCOV tracefile")
    ap.add_argument("--combined", type=Path,
                    help="combined LCOV tracefile for reconciliation")
    ap.add_argument("--junit", type=Path, required=True,
                    help="JUnit XML from the real test run")
    ap.add_argument("--min-line", type=int, default=90,
                    help="strict line floor: 100*lh must EXCEED min_line*lf (default 90)")
    ap.add_argument("--min-func", type=int, default=90,
                    help="strict function floor: 100*fnh must EXCEED min_func*fnf (default 90)")
    ap.add_argument("--min-branch", type=int, default=85,
                    help="branch floor: 100*brh >= min_branch*brf (default 85)")
    ap.add_argument("--require-inventory", action="append", dest="inventory",
                    metavar="TU", help="required production TU (repeatable; "
                    f"defaults to the M0-01 list: {', '.join(DEFAULT_INVENTORY)})")
    ap.add_argument("--expected-min-tests", type=int, default=None,
                    help="minimum total tests in the JUnit result")
    ap.add_argument("--allow-skips", action="store_true",
                    help="treat skipped tests as acceptable (default: finding)")
    args = ap.parse_args(argv)

    findings: list[str] = []

    # ---- misconfiguration checks (exit 2) ----
    inputs = {"--handwritten": args.handwritten, "--junit": args.junit}
    if args.generated:
        inputs["--generated"] = args.generated
    if args.combined:
        inputs["--combined"] = args.combined
    for flag, path in inputs.items():
        if not path.is_file():
            print(f"MISCONFIG: {flag} file not found: {path}", file=sys.stderr)
            return MISCONFIG
    if args.min_line < 0 or args.min_func < 0 or args.min_branch < 0:
        print("MISCONFIG: thresholds must be non-negative integers", file=sys.stderr)
        return MISCONFIG
    if args.expected_min_tests is not None and args.expected_min_tests < 0:
        print("MISCONFIG: --expected-min-tests must be >= 0", file=sys.stderr)
        return MISCONFIG

    try:
        hand = parse_lcov(args.handwritten)
        hand_t = totals(hand)
        junit = parse_junit(args.junit)
    except ValueError as e:
        print(f"MISCONFIG: {e}", file=sys.stderr)
        return MISCONFIG

    gen_t = comb_t = None
    if args.generated:
        try:
            gen_t = totals(parse_lcov(args.generated))
        except ValueError as e:
            print(f"MISCONFIG: {e}", file=sys.stderr)
            return MISCONFIG
    if args.combined:
        try:
            comb_t = totals(parse_lcov(args.combined))
        except ValueError as e:
            print(f"MISCONFIG: {e}", file=sys.stderr)
            return MISCONFIG

    # ---- handwritten gate (strict integer thresholds) ----
    # A floor of 0 disables that metric's numeric check (M0-01 carries
    # min_*: 0 — measurement truthfulness without numeric floors yet; the
    # >90 / >=85 floors arrive with SRC-01). When a floor IS set it is
    # enforced in pure integer arithmetic: strict floors (lines/functions)
    # require 100*hit > floor*found, so exactly 90.00% FAILS.
    print("== Handwritten coverage (the gate) ==")
    for metric, hit_key, found_key, floor, strict in (
        ("lines", "lh", "lf", args.min_line, True),
        ("functions", "fnh", "fnf", args.min_func, True),
        ("branches", "brh", "brf", args.min_branch, False),
    ):
        hit, found = hand_t[hit_key], hand_t[found_key]
        if floor == 0:
            print(f"  {metric:9s}: {pct_str(hit, found)}  (no numeric floor)  -> pass")
            continue
        ok = strict_gt(hit, found, floor) if strict else nonstrict_ge(hit, found, floor)
        relation = ">" if strict else ">="
        print(f"  {metric:9s}: {pct_str(hit, found)}  (gate {relation} {floor}%)  -> "
              f"{'pass' if ok else 'FAIL'}")
        if not ok:
            findings.append(
                f"handwritten {metric} {pct_str(hit, found)} does not satisfy "
                f"{relation} {floor}%")

    # ---- branch data presence ----
    # A capture run without branch instrumentation produces a tracefile with
    # zero BRF/BRH rollups (and zero BRDA details). Keying on the BRF total
    # (not on raw BRDA line presence) is the honest rule: branch data
    # "collected" means found-branch counters exist at all.
    brda_lines = 0
    with args.handwritten.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("BRDA:"):
                brda_lines += 1
    if hand_t["brf"] == 0:
        findings.append(
            "missing branch data: handwritten tracefile has zero branch-found (BRF) counters")
        print("  branch data: NONE (BRF=0, BRDA lines=0) — missing branch data is a FAIL")
    else:
        print(f"  branch data: present (BRF={hand_t['brf']}, BRDA lines={brda_lines})")

    # ---- inventory completeness ----
    inventory = args.inventory if args.inventory is not None else DEFAULT_INVENTORY
    print("== Inventory (M0-01: omitted TUs must be present) ==")
    for tu in inventory:
        matches = sorted(sf for sf in hand if sf.split("/")[-1] == tu)
        if not matches:
            findings.append(f"inventory: required TU absent from handwritten tracefile: {tu}")
            print(f"  {tu}: ABSENT -> FAIL")
        else:
            hits = ", ".join(
                f"{m} [{pct_str(hand[m]['lh'], hand[m]['lf'])}]" for m in matches
            )
            print(f"  {tu}: present ({hits})")

    # ---- zero-hit visibility (evidence that absent != zero) ----
    zero_hit = sorted(
        sf for sf, r in hand.items() if r["lf"] > 0 and r["lh"] == 0
    )
    print(f"  zero-hit handwritten TUs: {len(zero_hit)}")
    for sf in zero_hit:
        print(f"    {sf}")

    # ---- reconciliation: handwritten + generated == combined ----
    if comb_t is not None and gen_t is not None:
        print("== Reconciliation (handwritten + generated == combined) ==")
        for m in METRICS:
            lhs = hand_t[m] + gen_t[m]
            if lhs != comb_t[m]:
                findings.append(
                    f"reconciliation: {m} handwritten({hand_t[m]}) + "
                    f"generated({gen_t[m]}) = {lhs} != combined({comb_t[m]})")
                print(f"  {m}: {hand_t[m]} + {gen_t[m]} != {comb_t[m]} -> FAIL")
            else:
                print(f"  {m}: {comb_t[m]} ok")
    elif comb_t is not None or gen_t is not None:
        print("== Reconciliation: skipped (needs both --generated and --combined) ==")

    # ---- test outcomes ----
    print("== Test outcomes (JUnit) ==")
    print(f"  tests={junit['tests']} failures={junit['failures']} "
          f"errors={junit['errors']} skipped={junit['skipped']}")
    if junit["tests"] == 0:
        findings.append("JUnit reports zero tests executed")
    if junit["failures"] > 0 or junit["errors"] > 0:
        findings.append(
            f"JUnit reports {junit['failures']} failures and {junit['errors']} errors")
    if junit["skipped"] > 0 and not args.allow_skips:
        findings.append(f"JUnit reports {junit['skipped']} skipped tests (unexpected skips)")
    if args.expected_min_tests is not None and junit["tests"] < args.expected_min_tests:
        findings.append(
            f"JUnit reports {junit['tests']} tests < expected minimum "
            f"{args.expected_min_tests}")

    # ---- verdict ----
    summary = {
        "handwritten": {m: hand_t[m] for m in METRICS},
        "rates": {
            "line": round(100 * hand_t["lh"] / hand_t["lf"], 2) if hand_t["lf"] else None,
            "function": round(100 * hand_t["fnh"] / hand_t["fnf"], 2) if hand_t["fnf"] else None,
            "branch": round(100 * hand_t["brh"] / hand_t["brf"], 2) if hand_t["brf"] else None,
        },
        "junit": junit,
        "findings": findings,
    }
    print("== Result ==")
    print(json.dumps(summary))
    if findings:
        for f in findings:
            print(f"FINDING: {f}", file=sys.stderr)
        print(f"RESULT: FAIL ({len(findings)} finding(s))", file=sys.stderr)
        return FINDING
    print("RESULT: PASS", file=sys.stderr)
    return PASS


if __name__ == "__main__":
    sys.exit(main())
