# GitHub Pages: publish HTML coverage at `/coverage`

This document analyzes how to expose the LCOV `genhtml` output (already produced in [`.github/workflows/code-coverage.yml`](../.github/workflows/code-coverage.yml)) on the project GitHub Pages site at a path such as **`/coverage/`**, matching the public documentation URL configured in [`docs/_config.yml`](_config.yml) (`https://source.odbdesignserver.com`).

## Current state

| Piece | Behavior |
|--------|----------|
| **Pages site** | Built by [`.github/workflows/jekyll-gh-pages.yml`](../.github/workflows/jekyll-gh-pages.yml): Jekyll builds `./docs` → `./_site`, then `actions/upload-pages-artifact` + `actions/deploy-pages`. Trigger: **`push` to branch `release`** (and `workflow_dispatch`). |
| **Custom domain** | `url: https://source.odbdesignserver.com` in `_config.yml` (no `baseurl`), so the site root is the domain root. A folder `_site/coverage/` becomes **`https://source.odbdesignserver.com/coverage/`**. |
| **Coverage HTML** | `code-coverage` job runs `genhtml` into `coverage/html/`, uploads artifact `linux-coverage-report` (14 days). Same workflow also uploads to Codacy when tokens exist. |
| **Deployment model** | GitHub Pages stores **one** deployment per repo/environment. Each deploy **replaces** the previous published tree. Whatever you put in the uploaded artifact **is** the whole public site. |

### Constraint you must respect

If two workflows both call `deploy-pages` (or both publish the Pages artifact) without coordinating, **the last deployment wins** and can **wipe** content produced by the other (for example Jekyll-only deploy removing `/coverage/`, or coverage-only deploy removing the rest of the site).

Any solution must produce **a single combined artifact** (Jekyll output + `coverage/` tree) **or** serialize deploys so only one workflow ever publishes the full site.

---

## Goal

- **URL:** `https://source.odbdesignserver.com/coverage/` (and internal `genhtml` pages under that prefix).
- **Source of HTML:** existing `genhtml` step output (`coverage/html/` contents → deployed as `_site/coverage/`).
- **Optional:** Link from the Jekyll site (home or docs index) to “Test coverage” pointing at `/coverage/`.

---

## Solution A — Unified “release site” workflow (recommended)

**Idea:** One workflow runs on `release` (and optionally `workflow_dispatch`) that:

1. Runs the same coverage steps you already have (or calls a reusable workflow to avoid duplication).
2. Builds the Jekyll site to `_site` (same as today: `actions/jekyll-build-pages` with `source: ./docs`, `destination: ./_site`).
3. Copies the generated HTML report into the site tree: `mkdir -p _site/coverage && cp -r coverage/html/* _site/coverage/` (or `rsync`).
4. Runs Doxygen (if you re-enable copying API docs to `_site/api`) so the single artifact still matches your intended site.
5. Uploads **one** `upload-pages-artifact` and deploys with `deploy-pages`.

**Changes elsewhere:** Remove or disable the separate deploy in [`.github/workflows/jekyll-gh-pages.yml`](../.github/workflows/jekyll-gh-pages.yml) (or replace that file with this unified workflow) so there is **only one** publisher.

**Pros:** Predictable site layout; no race; coverage and docs always from the same commit on `release`.  
**Cons:** Larger workflow file or need for a reusable workflow chunk; `release` publishes only when this workflow runs (align triggers with your release process).

---

## Solution B — Keep two workflows; chain with `workflow_run`

**Idea:** Leave [`.github/workflows/code-coverage.yml`](../.github/workflows/code-coverage.yml) as the job that **generates** coverage and uploads `linux-coverage-report`.

Add a **new** workflow (or extend `jekyll-gh-pages.yml`) that:

- Triggers on `workflow_run` **completed** for workflow “Code Coverage”, filtering to **`release`** and **success** (and skip forks if needed).
- Checks out the **same commit** as the triggering run (`github.event.workflow_run.head_sha`).
- Downloads artifact `linux-coverage-report` from that workflow run (for example [`dawidd6/action-download-artifact`](https://github.com/dawidd6/action-download-artifact) or GitHub API with `actions:read`).
- Builds Jekyll to `_site`.
- Extracts coverage into `_site/coverage/` (from artifact path `coverage/html` → `_site/coverage/`).
- Uploads and deploys Pages once.

**Disable** the direct `on: push: branches: [release]` deploy in `jekyll-gh-pages.yml` **or** you still have two publishers and a race.

**Pros:** Coverage workflow stays focused; site rebuild always includes latest artifact from the finished coverage run.  
**Cons:** More moving parts (artifact download, `workflow_run` permissions, correct SHA); two pipelines to reason about; must remove duplicate Jekyll `push` trigger.

---

## Solution C — Coverage-only deploy (not suitable for your goal)

Deploying **only** `coverage/html` in a separate job would **replace** the entire Pages site with just coverage and **remove** the Jekyll site. **Do not** use a standalone `deploy-pages` that uploads only the coverage folder unless you merge it with the Jekyll build in the same artifact (which collapses to Solution A or B).

---

## Technical notes

### `genhtml` and subpaths

LCOV’s `genhtml` normally emits **relative** links between pages. Copying the contents of `coverage/html/` into `_site/coverage/` is the standard approach. If you ever see broken asset paths, check for a custom `--prefix` or hosting that strips trailing slashes; for GitHub Pages at `/coverage/index.html`, the usual layout works.

### Permissions

Publishing requires at least: `permissions: contents: read`, `pages: write`, `id-token: write` (as in `jekyll-gh-pages.yml`). Downloading artifacts from another workflow needs `actions: read` where applicable.

### Pull requests

Do **not** deploy Pages from PRs from forks. Restrict deploy steps to `release` (and/or internal PRs only if you explicitly want preview deploys).

### Discoverability

After implementation, add a visible link in Jekyll (for example in `docs/index.md` or the theme’s layout) to `/coverage/` so visitors find the report without knowing the URL.

---

## Suggested decision

| Priority | Choice |
|----------|--------|
| Simplicity and one pipeline | **Solution A** (unified workflow on `release`, retire duplicate Pages deploy). |
| Minimal change to `code-coverage.yml` | **Solution B** (`workflow_run` + artifact merge in Jekyll workflow; **remove** duplicate `push` deploy from Jekyll). |

---

## Implementation checklist (either A or B)

1. Ensure only **one** workflow performs `deploy-pages` for production.
2. After Jekyll build, copy `coverage/html/*` → `_site/coverage/`.
3. Confirm GitHub **Pages** settings: source = GitHub Actions (not legacy branch).
4. Confirm **custom domain** `source.odbdesignserver.com` DNS and TLS still point at GitHub Pages (no change usually needed for a new path).
5. Run once on `release` and open `https://source.odbdesignserver.com/coverage/`.
6. Add a Jekyll link to “Code coverage” → `/coverage/`.

---

## Related files

- [`.github/workflows/code-coverage.yml`](../.github/workflows/code-coverage.yml) — `lcov` / `genhtml`, artifact `linux-coverage-report`
- [`.github/workflows/jekyll-gh-pages.yml`](../.github/workflows/jekyll-gh-pages.yml) — Jekyll → Pages
- [`docs/_config.yml`](_config.yml) — site `url`, theme
