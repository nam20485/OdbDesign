#!/usr/bin/env python3
"""Ground-truth fixture harness for server-owned component connectivity (M0.3).

Derives, straight from the raw ODB++ source files of a fixed set of fixture
designs, the answer to "which pins of which components connect to which nets" and
writes it as golden JSON — the oracle that the C++ contract test and the two
client conformance tests (M3.3) compare against. See
docs/plan/component-connectivity.md §2, §4, §6.

Sources per step:

  * ``steps/<step>/eda/data`` — a net's identity for this purpose is the 0-based
    ordinal of its NET record (spec p.153: "The first NET record is net_num 0").
    TOP ``net_num`` refers to that ordinal.
  * ``steps/<step>/layers/comp_+_top/components`` and ``.../comp_+_bot/components``
    — top and bottom components live in SEPARATE files, so ``index`` (the 0-based
    ordinal of a CMP record within its own file) restarts at 0 per side.

Record grammar, as ComponentsFile.cpp parses it:

  CMP <pkg_ref> <x> <y> <rot> <mirror> <comp_name> <part_name> ;<attrs>;ID=<id>
  TOP <pin_index> <x> <y> <rot> <mirror> <net_num> <subnet_num> <toeprint_name>

``comp_name`` is the refDes. ``;ID=<id>`` is optional (spec p.30) and is read the
way ``AttributeLookupTable.cpp`` reads it — as the third ``;``-separated section
of the attribute token — so a record whose ID sits elsewhere is counted and
reported rather than quietly picked up by a looser regex.

``net_num``/``subnet_num`` of -1 (which the C++ holds in an ``unsigned int``, so
consumers see 4294967295) means unconnected: recorded as null, never as an
ordinal.

Components files and eda/data may be Unix-compressed (``.Z``), streamed here
through ``uncompress -c`` — nothing is ever written into the design tree. A stray
decompressed copy left in the tree by an earlier manual run does not shadow the
real source: only a regular file named ``components`` is taken as the source, and
``components`` is otherwise a directory. ``components2``/``components2.Z`` (the
ODB++ v7+ extended form) is deliberately not read; reading it would double-count
every record.

Output is deterministic: sorted keys, fixed array order, no timestamps, no
absolute paths, LF newlines. Running this twice yields byte-identical files.

stdlib only. Exits 0 when every requested fixture produced a golden file, 1 when
a fixture is missing or unparsable.
"""

import argparse
import gzip
import io
import json
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

DEFAULT_OUTPUT_DIR = REPO_ROOT / "OdbDesignTests" / "Fixtures" / "Connectivity"

# slug (=> "<slug>.golden.json"), design job directory relative to the repo root,
# and the step to read — None means "every step under steps/". A slug maps to one
# golden file, so a fixture whose steps all carry components must pin one.
FIXTURES = [
    ("sample_design", "designs/sample_design/sample_design", None),
    # Unpinned on purpose: its sibling steps/panel-layout holds comp_+_*/components
    # files with zero CMP records, so this fixture exercises the clean-skip path.
    ("Panel-g7162-31800_odb", "new_designs/Panel - g7162-31800_odb/Panel - g7162-31800_odb", None),
    ("200-40628_Rev1_v7", "new_designs/200-40628_Rev1_v7/200-40628_r1_v7_1_v7", None),
    ("350-41017_rev1_odbjob_v7", "new_designs/350-41017_rev1_odbjob_v7/350-41017_rev1_odbjob_v7",
     None),
]

SIDES = (("Top", "comp_+_top"), ("Bottom", "comp_+_bot"))

# The ODB++ source of record, in the order they appear in the wild. The first that
# is a *regular file* wins; a directory of the same name is an extraction artifact.
COMPONENT_FILE_NAMES = ("components", "components.Z", "components.gz")
EDA_DATA_NAMES = ("data", "data.Z", "data.gz")

NET_NAME_NONE = "$NONE$"

CMP_RECORD_TOKEN = "CMP"
TOP_RECORD_TOKEN = "TOP"
NET_RECORD_TOKEN = "NET"
COMMENT_TOKEN = "#"

CMP_FIELD_COUNT = 9
TOP_FIELD_COUNT = 9

UINT_MAX = 4294967295
UNCONNECTED_SENTINELS = frozenset({-1, UINT_MAX})
ID_SECTION_INDEX = 2  # third ";"-separated section of the CMP attribute token

# Attributes that carry no name=value pair are stored by the C++ under the bare
# token, so a CMP line may legitimately have more than the three sections.
MAX_REPORTED_ANOMALIES = 5


class FixtureError(Exception):
    """A fixture could not be located or one of its records did not parse."""


def read_lines(path):
    """Yield the text lines of an ODB++ source, uncompressing .Z/.gz on the fly."""
    if path.name.endswith(".Z"):
        stream, proc = _spawn_uncompress(path)
    elif path.name.endswith(".gz"):
        stream, proc = io.TextIOWrapper(gzip.open(str(path), "rb"), encoding="utf-8"), None
    else:
        stream, proc = path.open("r", encoding="utf-8"), None
    try:
        for line in stream:
            yield line.rstrip("\n").rstrip("\r")
    except UnicodeDecodeError as exc:
        raise FixtureError("%s is not UTF-8 (%s); refusing to guess at ground truth"
                           % (path, exc)) from exc
    finally:
        stream.close()
        if proc is not None:
            proc.wait()
            if proc.returncode != 0:
                raise FixtureError("uncompress -c failed for %s (exit %d)"
                                   % (path, proc.returncode))


def _spawn_uncompress(path):
    try:
        proc = subprocess.Popen(["uncompress", "-c", str(path)], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    except OSError as exc:
        raise FixtureError("cannot run `uncompress -c %s`: %s" % (path, exc)) from exc
    return io.TextIOWrapper(proc.stdout, encoding="utf-8"), proc


def resolve_source(directory, candidates):
    """First candidate in `directory` that is a regular file, else None."""
    for name in candidates:
        candidate = directory / name
        if candidate.is_file():
            return candidate
    return None


def parse_net_names(eda_data_path):
    """Net names, indexed by NET record ordinal — the net identity TOP refers to."""
    nets = []
    for raw in read_lines(eda_data_path):
        line = raw.strip()
        if not line or line.startswith(COMMENT_TOKEN):
            continue
        if not line.startswith(NET_RECORD_TOKEN + " "):
            continue
        # EdaDataFile.cpp reads the name with getline(stream, ';') and trims: it
        # is everything after "NET " up to the first ';'.
        nets.append(line[len(NET_RECORD_TOKEN):].split(";", 1)[0].strip())
    return nets


def parse_component_uid(attr_token):
    """Return (uid, found_id_somewhere) for a CMP attribute token.

    Mirrors AttributeLookupTable.cpp: the section before the first ';' is skipped,
    the next holds the attributes, the third holds ``ID=<id>`` when present.
    """
    sections = attr_token.split(";")
    uid = None
    if len(sections) > ID_SECTION_INDEX and "=" in sections[ID_SECTION_INDEX]:
        key, _, value = sections[ID_SECTION_INDEX].partition("=")
        if key == "ID":
            if not value.isdigit():
                raise FixtureError("component ID is not a number: %r" % sections[ID_SECTION_INDEX])
            uid = int(value)
    return uid, uid is not None or "ID=" in attr_token


def parse_uint(text, path, line_no, what, line):
    """An unsigned record field as the C++ would store it, so -1 becomes 4294967295."""
    try:
        value = int(text)
    except ValueError:
        raise FixtureError("%s:%d: %s is not a number: %s" % (path, line_no, what, line)) from None
    if value < 0:
        value += UINT_MAX + 1
    return value


def parse_components_file(path, side, nets, anomalies):
    """The component records of one board side, in file order."""
    components = []
    current = None

    for line_no, raw in enumerate(read_lines(path), start=1):
        line = raw.strip()
        if not line or line.startswith(COMMENT_TOKEN):
            continue
        fields = line.split()
        # Everything the parser is not interested in (UNITS=, the file's own ID=,
        # @/& attribute declarations, PRP, BOM, CPN) falls through the dispatch.
        token = fields[0]

        if token == CMP_RECORD_TOKEN:
            _require_fields(fields, CMP_FIELD_COUNT, path, line_no, line)
            uid, has_id_text = parse_component_uid(fields[8])
            if not fields[8].startswith(";"):
                anomalies["attr-token-without-leading-semicolon"].append(
                    "%s:%d %s" % (path.name, line_no, line))
            if has_id_text and uid is None:
                anomalies["id-outside-the-third-section"].append("%s:%d %s" % (path.name, line_no, line))
            current = {"refDes": fields[6], "side": side, "index": len(components),
                       "uid": uid, "pins": []}
            components.append(current)
        elif token == TOP_RECORD_TOKEN:
            if current is None:
                raise FixtureError("%s:%d: TOP record before any CMP record" % (path, line_no))
            _require_fields(fields, TOP_FIELD_COUNT, path, line_no, line)
            net_num = parse_uint(fields[6], path, line_no, "net_num", line)
            # subnet_num is an ordinal within its net, so it has no cross-check
            # against the NET record count the way net_num does.
            subnet_num = parse_uint(fields[7], path, line_no, "subnet_num", line)
            if net_num not in UNCONNECTED_SENTINELS and net_num >= len(nets):
                anomalies["net-ordinal-out-of-range"].append(
                    "%s:%d net_num %d with %d NET records" % (path.name, line_no, net_num, len(nets)))
            current["pins"].append({
                "ordinal": len(current["pins"]),
                "pinNumber": parse_uint(fields[1], path, line_no, "pin_index", line),
                "name": fields[8],
                "netOrdinal": None if net_num in UNCONNECTED_SENTINELS else net_num,
                "subnetOrdinal": None if subnet_num in UNCONNECTED_SENTINELS else subnet_num,
            })

    return components


def _require_fields(fields, expected, path, line_no, line):
    if len(fields) != expected:
        raise FixtureError("%s:%d: record has %d fields, expected %d: %s"
                           % (path, line_no, len(fields), expected, line))


def new_anomalies():
    buckets = ("attr-token-without-leading-semicolon", "id-outside-the-third-section",
               "net-ordinal-out-of-range")
    return {name: [] for name in buckets}


def report_anomalies(anomalies):
    for name, hits in sorted(anomalies.items()):
        if not hits:
            continue
        shown = ", ".join(hits[:MAX_REPORTED_ANOMALIES])
        more = "" if len(hits) <= MAX_REPORTED_ANOMALIES else " (+%d more)" % (len(hits) - MAX_REPORTED_ANOMALIES)
        print("  WARNING %-38s %6d  %s%s" % (name + ":", len(hits), shown, more))


def summarize(slug, design, step, nets, components, out_path):
    sides = {side: [c for c in components if c["side"] == side] for side, _ in SIDES}
    pins = [p for c in components for p in c["pins"]]
    unconnected = sum(1 for p in pins if p["netOrdinal"] is None)
    per_side_uids = {side: {c["uid"] for c in sides[side] if c["uid"] is not None} for side, _ in SIDES}
    top_uids, bottom_uids = per_side_uids["Top"], per_side_uids["Bottom"]
    collisions = {side: len({u for u in values if u < len(sides[side])})
                  for side, values in per_side_uids.items()}

    none_ordinal = next((i for i, name in enumerate(nets) if name == NET_NAME_NONE), None)
    none_pins = sum(1 for p in pins if none_ordinal is not None and p["netOrdinal"] == none_ordinal)
    connected = {p["netOrdinal"] for p in pins if p["netOrdinal"] is not None}

    ref_des_counts = {}
    for component in components:
        ref_des_counts[component["refDes"]] = ref_des_counts.get(component["refDes"], 0) + 1
    duplicates = sorted(k for k, v in ref_des_counts.items() if v > 1)

    print("[%s]" % slug)
    print("  design                          %s" % design)
    print("  step                            %s" % step)
    print("  golden file                     %s" % relative(out_path))
    print("  nets                            %d (%d touched by a pin%s)"
          % (len(nets), len(connected),
             "" if none_ordinal is None else ", incl. %d on %s" % (none_pins, NET_NAME_NONE)))
    print("  components                      %d = %d Top + %d Bottom"
          % (len(components), len(sides["Top"]), len(sides["Bottom"])))
    print("  pins (TOP records)              %d = %d Top + %d Bottom"
          % (len(pins), _pin_total(sides["Top"]), _pin_total(sides["Bottom"])))
    print("  unconnected pins (net_num -1)   %d / %d%s"
          % (unconnected, len(pins),
             "" if not pins else " (%.1f%%)" % (100.0 * unconnected / len(pins))))
    print("  CMP with ;ID=                   %s"
          % ", ".join("%s %d/%d%s" % (side.lower(), len(_side_uids(sides, side)), len(sides[side]),
                                       "" if not _side_uids(sides, side) else
                                       " (UID %d..%d)" % (min(_side_uids(sides, side)),
                                                           max(_side_uids(sides, side))))
                      for side, _ in SIDES))
    print("  Top/Bottom UID overlap          %d" % len(top_uids & bottom_uids))
    print("  UID equal to a same-side ordinal  %s"
          % ", ".join("%s %d / %d" % (side.lower(), collisions[side], len(sides[side]))
                      for side, _ in SIDES))
    if duplicates:
        print("  WARNING duplicate refDes          %d  %s%s"
              % (len(duplicates), ", ".join(duplicates[:5]),
                 "..." if len(duplicates) > 5 else ""))


def _side_uids(sides, side):
    return sorted(c["uid"] for c in sides[side] if c["uid"] is not None)


def _pin_total(components):
    return sum(len(c["pins"]) for c in components)


FULL_COMPONENT_LIMIT = 10000
NET_ROSTER_LIMIT = 50
SAMPLE_EDGE_COUNT = 50


def _unique_uids(records):
    counts = {}
    for c in records:
        if c["uid"] is not None:
            counts[c["uid"]] = counts.get(c["uid"], 0) + 1
    return {u for u, n in counts.items() if n == 1}


def _colliding(records):
    """Indices whose value equals some component's unique UID -- the ordinal/UID
    confusion the fixtures exist to pin down."""
    unique = _unique_uids(records)
    return [c for c in records if c["index"] in unique]


def _sample(components):
    picked = {}
    for side in sorted({c["side"] for c in components}):
        records = sorted((c for c in components if c["side"] == side), key=lambda c: c["index"])
        chosen = records[:SAMPLE_EDGE_COUNT] + records[-SAMPLE_EDGE_COUNT:] + _colliding(records)
        for c in chosen:
            picked[(c["side"], c["index"])] = c
    return [picked[k] for k in sorted(picked)]


def _aggregates(nets, components):
    sides = {}
    for c in components:
        sides[c["side"]] = sides.get(c["side"], 0) + 1
    pins = sum(len(c["pins"]) for c in components)
    return {
        "components_total": len(components),
        "components_by_side": dict(sorted(sides.items())),
        "pins_total": pins,
        "pins_connected": sum(1 for c in components for p in c["pins"] if p["netOrdinal"] is not None),
        "pins_unconnected": sum(1 for c in components for p in c["pins"] if p["netOrdinal"] is None),
        "nets_total": len(nets),
        "uids_present": sum(1 for c in components if c["uid"] is not None),
        "ordinal_uid_collisions": sum(len(_colliding([c for c in components if c["side"] == s]))
                                      for s in sorted(sides)),
    }


def build_golden(design, step, nets, components):
    full = len(components) <= FULL_COMPONENT_LIMIT
    golden = {
        "design": design,
        "step": step,
        # "sampled" means components/nets below are aggregates plus a deterministic
        # subset: never compare a server response against one for completeness.
        "fidelity": "full" if full else "sampled",
        "aggregates": _aggregates(nets, components),
        "nets": [{"ordinal": i, "name": name} for i, name in enumerate(nets if full else nets[:NET_ROSTER_LIMIT])],
        "components": components if full else _sample(components),
    }
    if not full:
        golden["nets_included"] = min(len(nets), NET_ROSTER_LIMIT)
    return golden


def render(golden):
    # compact: the two large legacy designs cost tens of MB when pretty-printed
    return json.dumps(golden, sort_keys=True, ensure_ascii=True, separators=(",", ":")) + "\n"


def relative(path):
    try:
        return Path("%s" % path.relative_to(REPO_ROOT))
    except ValueError:
        return path


def side_steps(design_root, pinned_step):
    if pinned_step:
        return [pinned_step]
    steps_dir = design_root / "steps"
    if not steps_dir.is_dir():
        raise FixtureError("%s has no steps/ directory" % relative(steps_dir))
    steps = sorted(p.name for p in steps_dir.iterdir() if p.is_dir() and (p / "layers").is_dir())
    if not steps:
        raise FixtureError("no step with a layers/ directory under %s" % relative(steps_dir))
    return steps


def run_fixture(slug, design_root_rel, pinned_step, output_dir):
    design_root = REPO_ROOT / design_root_rel
    if not design_root.is_dir():
        raise FixtureError("design root %s is not present" % design_root_rel)

    written = 0
    for step in side_steps(design_root, pinned_step):
        step_dir = design_root / "steps" / step
        eda_source = resolve_source(step_dir / "eda", EDA_DATA_NAMES)
        if eda_source is None:
            print("[%s] SKIP step %s: no eda/data source\n" % (slug, step))
            continue

        nets = parse_net_names(eda_source)
        anomalies = new_anomalies()
        components = []
        sources = {}
        for side, layer_name in SIDES:
            source = resolve_source(step_dir / "layers" / layer_name, COMPONENT_FILE_NAMES)
            sources[layer_name] = source
            if source is None:
                continue
            components.extend(parse_components_file(source, side, nets, anomalies))

        if not components:
            print("[%s] SKIP step %s: 0 CMP records (%s)\n"
                  % (slug, step, ", ".join("%s: %s" % (layer, _name_or(p))
                                           for layer, p in sources.items())))
            continue

        out_path = output_dir / ("%s.golden.json" % slug)
        if written:
            raise FixtureError("fixture %s has components in more than one step, so one %s.golden.json"
                               " cannot hold them all — pin a step" % (slug, slug))
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(render(build_golden(design_root.name, step, nets, components)),
                            encoding="utf-8", newline="\n")

        summarize(slug, design_root.name, step, nets, components, out_path)
        report_anomalies(anomalies)
        print()
        written += 1

    if not written:
        raise FixtureError("fixture %s produced no golden file" % slug)


def _name_or(path):
    return path.name if path else "missing"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, default=DEFAULT_OUTPUT_DIR,
                        help="directory for the golden files (default: %s)"
                             % relative(DEFAULT_OUTPUT_DIR))
    parser.add_argument("--fixture", action="append", dest="only", metavar="SLUG",
                        help="run only this slug (repeatable); default: all")
    args = parser.parse_args(argv)

    failures = []
    for slug, design_root_rel, pinned_step in FIXTURES:
        if args.only and slug not in args.only:
            continue
        try:
            run_fixture(slug, design_root_rel, pinned_step, args.out)
        except FixtureError as exc:
            failures.append(str(exc))
            print("[%s] ERROR %s\n" % (slug, exc))

    if failures:
        print("%d fixture(s) failed:" % len(failures))
        for failure in failures:
            print("  %s" % failure)
        return 1
    print("wrote golden files to %s" % relative(args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
