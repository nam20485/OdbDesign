#!/usr/bin/env python3
"""Route-drift check: Crow C++ route registrations vs. the OpenAPI spec.

Extracts every registered route path from

  * OdbDesignServer/Controllers/*.cpp
  * OdbDesignServer/OdbServerAppBase.cpp

via the two registration forms used in the code base:

  * CROW_ROUTE(m_serverApp.crow_app(), "/path/<string>")   (Crow macro; first
    argument is the app expression, second the path string literal)
  * register_route_handler("/path", ...)                   (RouteController
    helper; used by HealthCheckController)

and compares them (structurally: Crow ``<string>``/``<int>``/... parameters are
treated as OpenAPI ``{param}`` placeholders; trailing slashes stripped) with the
``paths:`` keys of swagger/odbdesign-server-0.9-swagger.yaml.

Commented-out registrations (``//CROW_ROUTE ...``) are ignored.

Exits 0 when the code and spec agree (modulo ALLOWLIST), 1 on any drift,
2 when an input file is missing/unreadable (distinct from drift so CI can
tell misconfiguration from a real finding).

stdlib only.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

SOURCES = sorted((REPO_ROOT / "OdbDesignServer" / "Controllers").glob("*.cpp")) + [
    REPO_ROOT / "OdbDesignServer" / "OdbServerAppBase.cpp"
]

SWAGGER_PATH = REPO_ROOT / "swagger" / "odbdesign-server-0.9-swagger.yaml"

# Routes registered in code but intentionally absent from the OpenAPI spec.
# TODO(drift): OdbServerAppBase.cpp registers two *minimal* legacy probe routes
#   that predate the HealthCheckController endpoints documented in the spec
#   (/healthz/live, /healthz/started, /healthz/ready). If they are meant to
#   stay public, document them in the spec (or remove the routes); either way
#   resolve the drift and drop the allowlist entries.
ALLOWLIST_CODE_ONLY = {
    "/healthz",
    "/ready",
}

# Routes documented in the spec but intentionally absent from the code (none
# currently).
ALLOWLIST_SPEC_ONLY = set()

# Crow typed path-parameter placeholders -> structural wildcard.
CROW_PARAM_RE = re.compile(r"<[^<>]+>")
# Spec path parameter placeholders -> structural wildcard.
SPEC_PARAM_RE = re.compile(r"\{[^{}]+\}")

# CROW_ROUTE(<app-expression>, "<path>")  -- app expression contains no comma.
CROW_ROUTE_RE = re.compile(r'CROW_ROUTE\s*\(\s*[^,()]+(?:\([^()]*\))?\s*,\s*"([^"]+)"')
# register_route_handler("<path>", <handler>)
REGISTER_ROUTE_RE = re.compile(r'register_route_handler\s*\(\s*"([^"]+)"')


def normalize(path: str) -> str:
    """Normalize a route path for structural comparison."""
    path = CROW_PARAM_RE.sub("{}", path)
    path = SPEC_PARAM_RE.sub("{}", path)
    return path.rstrip("/")


def read_or_exit(path: Path) -> str:
    """Read a file, exiting with code 2 (not the drift code) when unreadable."""
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        print(f"error: cannot read {path}: {exc}", file=sys.stderr)
        sys.exit(2)


def extract_code_routes() -> dict:
    """Return {normalized_path: "raw_path (file:line)"} for registered routes."""
    routes = {}
    for src in SOURCES:
        for lineno, line in enumerate(read_or_exit(src).splitlines(), start=1):
            # Skip commented-out registrations.
            if line.lstrip().startswith("//"):
                continue
            for regex in (CROW_ROUTE_RE, REGISTER_ROUTE_RE):
                for raw in regex.findall(line):
                    routes.setdefault(normalize(raw), f"{raw} ({src.name}:{lineno})")
    return routes


def extract_spec_paths() -> dict:
    """Return {normalized_path: raw_path} from the swagger YAML `paths:` section.

    Conservative stdlib parse: inside the top-level ``paths:`` mapping, path
    keys are the lines indented by exactly two spaces that start with ``/``.
    """
    paths = {}
    in_paths = False
    for line in read_or_exit(SWAGGER_PATH).splitlines():
        if re.fullmatch(r"paths:\s*", line):
            in_paths = True
            continue
        if in_paths:
            if line and not line[0].isspace():  # next top-level key: section over
                break
            match = re.match(r"^  (/\S*):\s*$", line)
            if match:
                paths.setdefault(normalize(match.group(1)), match.group(1))
    return paths


def main() -> int:
    code_routes = extract_code_routes()
    spec_paths = extract_spec_paths()

    missing_from_spec = sorted(set(code_routes) - set(spec_paths))
    missing_from_code = sorted(set(spec_paths) - set(code_routes))

    unallowlisted_spec = [p for p in missing_from_spec if p not in ALLOWLIST_CODE_ONLY]
    unallowlisted_code = [p for p in missing_from_code if p not in ALLOWLIST_SPEC_ONLY]

    print(f"routes registered in code: {len(code_routes)}")
    print(f"paths documented in spec:  {len(spec_paths)}")

    if missing_from_spec:
        print("\nin code but missing from spec:")
        for path in missing_from_spec:
            note = "  (ALLOWLISTED)" if path in ALLOWLIST_CODE_ONLY else ""
            print(f"  {code_routes[path]}{note}")

    if missing_from_code:
        print("\nin spec but missing from code:")
        for path in missing_from_code:
            note = "  (ALLOWLISTED)" if path in ALLOWLIST_SPEC_ONLY else ""
            print(f"  {spec_paths[path]}{note}")

    if unallowlisted_spec or unallowlisted_code:
        print("\nroute drift detected (see above); update the spec or code,", end=" ")
        print("or add an explicit ALLOWLIST entry with a TODO comment.")
        return 1

    print("\nOK: code routes and OpenAPI spec paths agree (modulo allowlist).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
