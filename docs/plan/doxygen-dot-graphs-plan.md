# Plan: Enable dot-based graphs in the Doxygen API docs

Date: 2026-11-20
Status: implemented 2026-11-20 on `nam/doxygen-dot-graphs` → PR → `nam20485` (merge commit only)
Branch when implemented: `nam/doxygen-dot-graphs` → PR → `nam20485` (merge commit only)

## Implementation verification (local, 2026-11-20)

- `./scripts/generate-doxygen-docs.sh` exit 0 in **4.6 s wall** (user 37.8 s —
  DOT_MULTI_TARGETS + parallel dot beat the 2-6 min estimate by ~50×).
- **1555 SVG graphs** emitted; `docs/api/graph_legend.html` present.
- Hub class page (`classOdb_1_1Lib_1_1ProductModel_1_1Design.html`):
  inheritance diagram embedded as interactive-SVG iframe
  (`...__inherit__graph.svg`), 33 call/caller-graph sections; controller
  pages carry "Here is the call graph" sections.
- Real render check (headless google-chrome): the SVG parses to a DOM with
  22 shape elements; full-page screenshot pixel analysis shows drawn
  diagram content (13k+ non-background px, 496 colors in the densest band).
- 11 doxygen warnings, all PRE-EXISTING doc-comment issues (`\class for` in
  RgbColor.h, "no matching class member" in FileArchive.cpp/ArchiveExtractor.cpp,
  unknown `@why` command) — none graph-related; out of scope.
- Note: output paths are `docs/api/*` (HTML_OUTPUT=api), not `docs/api/html/*`
  as §Verification item 1 drafted — assertions used the real paths.

## Goal

Turn on the full set of Graphviz (`dot`)-generated diagrams in the published API
reference (`/api` on the GitHub Pages site): directory graphs, include /
included-by graphs, class inheritance hierarchy, collaboration graphs, group
graphs, and per-function call / caller graphs.

## Current state

- Doxygen config: `OdbDesignLib/doxygen.conf` (Doxyfile 1.9.8).
  - `HAVE_DOT = NO` (line 2527). This single flag disables **every** dot-based
    graph regardless of the per-graph toggles.
  - Already `YES` but inert while `HAVE_DOT = NO`: `DIRECTORY_GRAPH`,
    `CLASS_GRAPH`, `COLLABORATION_GRAPH`, `GROUP_GRAPHS`, `INCLUDE_GRAPH`,
    `INCLUDED_BY_GRAPH`, `GRAPHICAL_HIERARCHY`.
  - Explicitly `NO` and needing a flip: `CALL_GRAPH` (2695), `CALLER_GRAPH`
    (2707), `TEMPLATE_RELATIONS` (2660).
  - Rendering knobs at defaults: `DOT_IMAGE_FORMAT = png`, `INTERACTIVE_SVG =
    NO`, `DOT_PATH` empty, `DOT_NUM_THREADS = 0`, `DOT_GRAPH_MAX_NODES = 50`,
    `DOT_MULTI_TARGETS = NO`.
- Generation happens only in CI: `.github/workflows/jekyll-gh-pages.yml`
  installs doxygen (`sudo apt install -y doxygen`, line 62) and runs
  `doxygen ./OdbDesignLib/doxygen.conf` (line 65) from the workspace root.
  Output lands in `docs/api/` (`OUTPUT_DIRECTORY = docs`, `HTML_OUTPUT = api`),
  which is gitignored (`.gitignore` line 26); the workflow copies it into the
  Jekyll `_site/api/` (lines 90-94) and deploys to Pages.
- Workflow triggers: push to `release`, PRs to the five long-lived branches,
  weekly cron (Mon 06:00 UTC), and `workflow_dispatch`. PR runs are
  build-validation only (no deploy), so the PR that lands this change itself
  exercises the new graph generation end to end.
- Input set: `OdbDesignLib`, `OdbDesignServer`, `OdbDesignApp`, `Utils`
  (tests deliberately excluded). ~161 source files, ~91 headers — a moderate
  load for dot; call/caller graphs will number in the low thousands.

## Local vs CI parity (the direct answer)

Yes — the generation stage is already environment-neutral:

- It is a single command, `doxygen ./OdbDesignLib/doxygen.conf`, run from the
  repo root. There is **no** CMake/`find_package(Doxygen)` involvement
  (verified: no doxygen references in any `CMakeLists.txt`), so no build tree,
  vcpkg, or preset is needed.
- Versions match: local `doxygen` is 1.9.8 (`/usr/bin/doxygen`), same major
  line the ubuntu-24.04 runner installs via apt; the Doxyfile itself is
  formatted for 1.9.8. `dot` (graphviz 2.42.4) is already on this machine.

Two parity gaps this plan closes:

1. CI never installs `graphviz` — the apt `doxygen` package does not
   hard-depend on it, so with `HAVE_DOT = YES` CI would silently emit
   warning "dot ... not found" and produce graph-less HTML while local runs
   (which have `dot`) produce graphs. Fix: install `graphviz` explicitly.
2. The command exists only inside the workflow YAML. Add
   `scripts/generate-doxygen-docs.sh` as the one entry point both the workflow
   and local runs call, so there is exactly one way to generate `/api` docs.

The Jekyll half of the workflow (`actions/jekyll-build-pages`) is CI-only and
containerized, but it does not transform `/api`; it only copies
`docs/api/. → _site/api/`, so parity for the Doxygen stage is what matters.

## Changes

### 1. `OdbDesignLib/doxygen.conf`

| Tag | Current | New | Why |
|---|---|---|---|
| `HAVE_DOT` | NO | YES | Master switch; enables all dot graphs |
| `DOT_PATH` | empty | empty | `dot` resolves via PATH in both CI and local (script asserts it) |
| `CALL_GRAPH` | NO | YES | Callee graphs on function pages |
| `CALLER_GRAPH` | NO | YES | Caller graphs on function pages |
| `TEMPLATE_RELATIONS` | NO | YES | Template inheritance/instrel edges — part of "fancy" |
| `DOT_IMAGE_FORMAT` | png | svg | Crisper, tiny text-based images; prerequisite for interactive SVG |
| `INTERACTIVE_SVG` | NO | YES | Zoomable/pannable graphs in the browser |
| `DOT_MULTI_TARGETS` | NO | YES | Doxygen 1.9.8 batches multiple graphs per dot invocation — big win with thousands of call graphs |
| `DOT_GRAPH_MAX_NODES` | 50 | 100 | Hub classes (e.g. `Design`, `FileArchive`) truncate caller graphs at 50; 100 keeps them informative while bounded |
| `DOT_NUM_THREADS` | 0 | 0 | Already "all cores"; leave |

Unchanged (already correct): `DIRECTORY_GRAPH`, `CLASS_GRAPH`,
`COLLABORATION_GRAPH`, `GROUP_GRAPHS`, `INCLUDE_GRAPH`, `INCLUDED_BY_GRAPH`,
`GRAPHICAL_HIERARCHY`, `GENERATE_LEGEND`, `DOT_CLEANUP`, `MAX_DOT_GRAPH_DEPTH`
(0 = unlimited, node cap does the bounding).

### 2. `.github/workflows/jekyll-gh-pages.yml`

- Line 62: `sudo apt install -y doxygen` → `sudo apt install -y doxygen graphviz`.
- Line 65: replace the inline `doxygen ./OdbDesignLib/doxygen.conf` with
  `./scripts/generate-doxygen-docs.sh`.
- No structural changes: the existing copy step already carries
  `docs/api/.` (including `*.svg`) into `_site/api/`, and Harden-Runner is in
  `audit` mode so the extra apt fetch of graphviz is unaffected.

### 3. New `scripts/generate-doxygen-docs.sh` (single local/CI entry point)

```bash
#!/usr/bin/env bash
# Generate the Doxygen API reference into docs/api/ (gitignored).
# Same entry point for CI (.github/workflows/jekyll-gh-pages.yml) and local use.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"   # doxygen.conf paths are cwd-relative
command -v doxygen >/dev/null || { echo "error: doxygen not installed" >&2; exit 1; }
command -v dot      >/dev/null || { echo "error: graphviz 'dot' not installed" >&2; exit 1; }
echo "doxygen $(doxygen --version), $(dot -V 2>&1)"
doxygen ./OdbDesignLib/doxygen.conf
```

Fail loudly when `dot` is missing rather than letting doxygen degrade to
graph-less output (its "dot not found" is only a warning).

## Expected impact

- Build time: dot rendering for ~161 files with call+caller graphs on
  `EXTRACT_ALL = YES`. With `DOT_MULTI_TARGETS = YES` and
  `DOT_NUM_THREADS = 0` expect roughly 2-6 min on a standard runner (vs ~30 s
  today). Acceptable for the Pages workflow; PR validation lane uses its own
  concurrency group (`pages-pr-N`) so deploys never queue behind it.
- Site size: today `docs/api` is a few MB; with several thousand SVG graphs
  expect tens of MB (SVG is text and compresses well server-side). Far below
  the GitHub Pages 1 GB site limit.

## Verification (real-outcome checks, per the no-grep-pseudo-validation rule)

1. Local: run `./scripts/generate-doxygen-docs.sh`; assert exit code 0 and
   that graph artifacts were actually emitted, e.g.
   `find docs/api -name '*.svg' | wc -l` returns a non-trivial count and
   `docs/api/graph_legend.html` exists (output is `docs/api/*` directly —
   `OUTPUT_DIRECTORY = docs`, `HTML_OUTPUT = api`; there is no `html/`
   level).
2. Round-trip in a browser: open `docs/api/index.html` locally, navigate
   to a hub class page (e.g. `classOdb_1_1Lib_1_1ProductModel_1_1Design.html`)
   and confirm the collaboration/inheritance SVG renders and zooms; open a
   method page and confirm caller/callee graph presence.
3. CI: the PR to `nam20485` itself triggers the build-validation lane, which
   runs the same script on ubuntu-24.04 — a real generation run gating the PR.
4. Post-merge: after the change reaches `release` (or a manual
   `workflow_dispatch` against `release`), `curl -fsSI` a known graph asset on
   the live Pages site (e.g. `/api/graph_legend.html` and one class SVG such
   as `/api/classOdb_1_1Lib_1_1ProductModel_1_1Design__inherit__graph.svg`)
   and expect HTTP 200 — a real round-trip, not a config grep.
5. Parity spot-check: diff a file listing of `docs/api` generated locally vs
   the workflow artifact (optional; catches env-driven divergence like a
   runner graphviz too old for a feature — doxygen emits warnings in step 1's
   version echo if so).

## Risks / mitigations

- **Silent graph loss if `dot` missing** → wrapper script hard-fails; CI
  installs graphviz explicitly.
- **Giant hub-class caller graphs** → `DOT_GRAPH_MAX_NODES = 100` cap; excess
  nodes collapse into truncation boxes.
- **PR-lane slowdown** → own concurrency group already isolates deploys;
  `DOT_MULTI_TARGETS` + thread count keep it bounded.
- **Doxygen/graphviz version skew between local Debian and the runner** →
  both currently doxygen 1.9.8 and graphviz ≥ 2.42; the script prints versions
  on every run so skew is visible in CI logs.
- **INTERACTIVE_SVG browser quirks** → standard doxygen feature, no known
  issues in current browsers; reverting to static PNG is a one-tag rollback.

## Out of scope (follow-ups if wanted)

- `GENERATE_LATEX` / PDF with graphs (currently NO; large build-cost increase).
- Prebuilt doxygen/graphviz pinning via a container image instead of apt.
- Per-`\callgraph`/`\callergraph` command opt-in model (inverse approach:
  graphs only where asked) if build time ever becomes a problem.
