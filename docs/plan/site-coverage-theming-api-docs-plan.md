# Plan: coverage-page theming + drop test classes from API docs

Status: APPROVED 2026-11-16 with user feedback: the branch index must be a
*self-explaining picker*, not a bare 2-row table the user mistook for the
whole report — add explanatory copy, per-branch coverage numbers, colored
chips, and explicit "Open report" actions, in the site chrome.

IMPLEMENTED same day. Verification (all real-outcome, run pre-PR):
- Index generator extracted from the workflow and executed standalone with a
  stubbed `gh`: all content assertions pass (chrome wrapper, stylesheet link,
  lead copy, chips parsed from `coverage_summary.txt` incl. 72.4 %→red /
  91.2 %→green classification, short SHA, date, placeholder path, artifact
  copy layout identical to the live tree).
- `genhtml --css-file` round-trip in ubuntu-24.04 container (lcov 2.0-1,
  CI's version): output `gcov.css` byte-identical to `scripts/gcov-dark.css`,
  pages link it, `snow-dark.png` overwrite verified.
- Doxygen 1.9.8 (CI's version) in container: new INPUT →
  `namespaceOdb_1_1Test.html` absent, `namespaceOdb.html` present, zero
  `*Tests*` pages; control run with old INPUT + `OdbDesignTests/` present
  reproduces the page (112 test pages) — assertion is sensitive.
- Bars: lcov renders coverage bars as `ruby.png`/`snow.png` images; the CSS
  cannot recolor them, so `snow-dark.png` (panel-colored replacement for the
  white "uncovered" segment) is copied over `snow.png` post-genhtml.

## 1. Problem A — `/coverage/` looks lame

**Diagnosis (nothing regressed in the coverage pipeline — the site around it changed):**

- `jekyll-gh-pages.yml` `Add per-branch coverage reports` step generates
  `coverage/index.html` as **bare inline HTML** (heredoc, lines ~159–181):
  no stylesheet link, `<table border="1">`. It is written straight into
  `_site/` **after** the Jekyll build, so it never touches the slate theme.
- The per-branch reports (`/coverage/<branch>/`) are **stock `genhtml` (lcov)
  output** baked into the coverage artifact at run time
  (`code-coverage.yml` `Generate Coverage Report`, lines ~198–203), carrying
  lcov's own light-gray `gcov.css` (~2003 look). All assets (gcov.css,
  glass.png, …) are served fine — verified 200s — it is purely a styling gap.
- #612 restructured the site into the link-hub and #621 gave it the slate dark
  chrome (`docs/assets/css/style.scss`, #4d4d4d panels / #f2f2f2 text). The
  Jekyll-rendered pages now look polished; `/coverage/` never participated in
  the chrome, so the contrast became glaring. The weekly Monday refresh
  (2026-11-16 06:29 UTC) republished the tree in the same raw state.

## 2. Problem B — test classes in `/api/`

`OdbDesignLib/doxygen.conf` `INPUT` (lines 946–950) includes
`./OdbDesignTests`, so every `Odb::Test` fixture/test class gets a page
(`namespaceOdb_1_1Test.html` — the URL reported is just an anchor inside that
page). Verified: no `Odb::Test` references exist outside `OdbDesignTests/`,
so dropping the directory removes the whole namespace cleanly with no dangling
cross-references.

## 3. Changes (one batched PR, `nam/<feature>` → `nam20485`)

1. **`OdbDesignLib/doxygen.conf`** — remove `./OdbDesignTests` from `INPUT`.
   Optional: drop ", tests" from `PROJECT_BRIEF` to match the new doc scope.
2. **New `scripts/gcov-dark.css`** — dark-theme replacement for lcov's
   `gcov.css`, palette matched to the site chrome: page background dark slate
   gradient, data tables #4d4d4d panels, #f2f2f2 text, borders #565656;
   coverage colors stay green/amber/red (translucent fills over dark, dark
   text on bright fills) so hit/miss semantics are unchanged.
3. **`code-coverage.yml`** — `genhtml … --css-file scripts/gcov-dark.css`
   (genhtml copies it into the artifact as `gcov.css`; every generated page
   inherits it; future artifacts are themed at the source).
4. **`jekyll-gh-pages.yml`** — the branch index becomes a self-explaining
   picker in the site chrome (slate wrapper divs + `<link>` to
   `/assets/css/style.css` + embedded scoped CSS):
   - Lead copy states plainly: full per-branch lcov reports live one click
     away (directory tree, per-file, line-by-line); this page is the hub.
   - Fancy table: big branch-name link + one-line description, commit + run
     date, **Lines % and Functions % chips parsed from the artifact's
     `coverage_summary.txt`** (lcov thresholds: green ≥ 90, amber ≥ 75,
     red < 75, gray = missing), explicit "Open report →" action cell, row
     hover highlight.
   - Placeholder branch pages get the same chrome; footnote keeps the
     "last checked" timestamp and links back to the site home.

Already-published artifacts: the currently-live per-branch reports keep the
old CSS until each branch's next coverage run; the index re-themes on the
next deploy.

## 4. Verification (real-outcome, not grep-of-config)

- Doxygen: run `doxygen ./OdbDesignLib/doxygen.conf` locally; assert the
  generated tree **lacks** `namespaceOdb_1_1Test.html` (and test class pages)
  and **has** `namespaceOdb.html` — artifact comparison of real generator
  output. Same assertion holds in the PR validation lane implicitly (site
  build succeeds; page absent from `/api` after deploy).
- Coverage theming: PR lane builds the site (catches heredoc/Liquid
  breakage). Live check after promotion: `curl` the pages, confirm
  `/assets/css/style.css` link present in `/coverage/` and `gcov.css` served
  with dark-theme content; user eyeballs (established deploy-verify loop).
- Deployment reality: the site builds from **release** (push + weekly
  schedule). These fixes reach production only via the usual promotion chain;
  the branch-index re-theme lands on the next deploy, the LCOV re-theme on
  each branch's next coverage run.

## 5. Open question (aesthetic, user's call)

LCOV palette above is matched to the current site chrome (#4d4d4d panels /
#f2f2f2 text). Alternative: use the "next stop" #565656 panel color being
considered for the site. Default: match what is live today.
