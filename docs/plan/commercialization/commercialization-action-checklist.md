# Commercialization Action Checklist — every open item, step by step

*Derived 2026-11-08 from [OdbDesign_Commercialization_Readiness_Guide.md](OdbDesign_Commercialization_Readiness_Guide.md) after the execution round (PR #610 + private overlay) and the 2026-11-08 feedback round. Every guide item whose status is **not** `Complete` appears here, with a reference back to the parent item, a description, and concrete steps. Items are grouped by urgency: Section 0 gates today's Keiron reply; Sections A–D are the remaining program.*

**Status legend:** ☐ todo · ◐ in progress · ✓ done (remove from this list when closed)

---

## Section 0 — SHORT-LIST: resolve before sending the Keiron reply (today, 2026-11-08)

**Hard blockers: none.** The reply (two drafts to choose from: `keiron-reply-draft-friendly-2026-11-08.md` and `keiron-reply-draft-formal-2026-11-08.md`) is deliberately an information-gathering response — no binding terms, no prices, no entity signature required. The five items below are the only decisions that shape what you press "send" on:

| # | Decision | Recommendation | Time |
|---|---|---|---|
| S1 | **Signature block** — personal, or The Intel Agency? (parent 1.1 / A1) | Sign personally as *maintainer/author of OdbDesign* — the entity's internal IP license (A1) isn't executed yet; switching the contracting name later is normal ("agreements will be with our company, The Intel Agency"). | 1 min |
| S2 | **Evaluation framing** — licensed eval binaries (parent 2.4, decided + amended 2026-11-08) | Settled: NDA → 30-day watermarked evaluation license (loopback-only for sidecars) → commercial license. Both email drafts carry this framing. Nothing further before sending — the eval agreement paper (A6) is needed only when Keiron actually requests the evaluation. | 0 min |
| S3 | **No pricing in the first email** (parent 2.3) | Confirmed approach: gather scoping info first; send tailored indicative pricing after the call. Pricing decision (B2) is needed before the *quote*, not before the *reply*. | 0 min |
| S4 | **Scheduling link works for EU timezones** | Verify the Google Calendar link (docs/README.md) once; note your availability for CET mornings (= early PST) so Monday slots are bookable. | 2 min |
| S5 | **Send window** | Before **23:00 PST Sunday = 08:00 CET Monday**; send this evening so it's first-in-inbox at 08:00. | — |

Also acceptable to attach: nothing. The email is self-contained; the commercial overview PDF (C6) comes after scoping.

---

## Section A — Legal & corporate (user + counsel)

### A1. Execute The Intel Agency as licensor entity ☐
**Parent:** guide 1.1 · **Decided:** 2026-11-08 (The Intel Agency; WA LLC later if ARR > ~$50k)

Until this is done, nothing binding may be signed by the entity. Steps:

1. Review (optionally with counsel) the 1-page **Internal IP License draft** below.
2. Fill the bracketed placeholders; print two copies.
3. Sign both (you are both parties — sign as Nathan Miller, and for The Intel Agency if you are its authorized signer).
4. Scan + store with the other corporate records; log the date in the provenance memo (A2).
5. Update contract templates (when drafted, A7) so the contracting party is "The Intel Agency"; email signatures may reference the entity from then on.

<details><summary><b>Internal IP License — 1-page draft</b> (template; counsel review recommended)</summary>

```
INTERNAL IP LICENSE AND COMMERCIALIZATION AGREEMENT

Effective date: [DATE]

Between: Nathan Miller, an individual ("Licensor")
And: The Intel Agency, a [WA LLC / entity type] ("Licensee")
     (each a "Party", together "the Parties")

1. DEFINITIONS. "OdbDesign" means the software known as OdbDesign,
   including the repositories at github.com/nam20485/OdbDesign and
   github.com/nam20485/OdbDesign-Commercial, all associated documentation,
   build systems, trademarks, and derivative works thereof.

2. LICENSE GRANT. Licensor grants Licensee an exclusive, worldwide,
   royalty-free, perpetual license to:
   (a) reproduce, adapt, and distribute OdbDesign in binary and source form;
   (b) sublicense OdbDesign to third parties under commercial terms of
       Licensee's choosing, including proprietary (closed-source) terms; and
   (c) enforce the copyrights in OdbDesign against infringers, with Licensor
       retaining a join interest in any such action.

3. RESERVED RIGHTS. Licensor reserves the right to continue publishing and
   distributing OdbDesign under the GNU Affero General Public License v3.0
   (the "Open-Source Distribution") and to maintain the public repository.
   The reserved Open-Source Distribution does not diminish Licensee's
   commercial sublicensing rights.

4. OWNERSHIP. Title and copyright in OdbDesign remain with Licensor, subject
   to the license in §2. Each Party will execute reasonable further
   assurances to perfect Licensee's rights.

5. REVENUE. Licensee shall pay Licensor [100%] of net receipts from
   OdbDesign commercial licensing, less third-party costs of sale.
   [Or as otherwise agreed in writing.]

6. TERM. Perpetual; §2 survives any termination.

Signed:

_____________________________          _____________________________
Nathan Miller, Licensor               The Intel Agency, Licensee
Date:                                 By: Nathan Miller, [Title]
                                      Date:
```
</details>

### A2. Write the CIIAA separation provenance memo ☐
**Parent:** guide 1.2 · **Resolved by fact pattern 2026-11-08; memo is cheap insurance**

1. Create a private doc (password manager or the private repo — **not** the public repo) titled "OdbDesign provenance record".
2. Record, in dated bullets: OdbDesign development start (≈2 years before Cincinnatus employment began [DATE]); all Mercor/Cincinnatus work performed on the employer-issued corporate ChromeOS laptop; OdbDesign work performed exclusively on personal hardware and personal network, on personal time (WA RCW 49.44.140 carve-out: no employer equipment, supplies, facilities, or time).
3. Note the repository history as corroborating evidence (commit timestamps/devices predate employment).
4. Revisit/append whenever the fact pattern could change.

### A3. Enable CLA Assistant (future contributions) ☐
**Parent:** guide 1.3(a) · **What it is:** a free GitHub integration that presents your CLA to contributors on their first PR, records their acceptance into a repo you control, and enforces it as a PR status check — you keep the CLA text, it automates capture.

1. In a browser, go to **https://cla-assistant.io** and click **Sign in with GitHub** (authorize with the `nam20485` account).
2. If you have no signature-storage repo yet, create an empty **private** repository, e.g. `nam20485/cla-signatures`.
3. On the CLA Assistant dashboard click **Configure a repository** (or "Add repository").
4. In **"Repository to integrate CLA Assistant"** enter: `nam20485/OdbDesign`.
5. In **"Repository to store the CLA signatures"** enter: `nam20485/cla-signatures`.
6. Point **"CLA document location"** at the committed file: `https://github.com/nam20485/OdbDesign/blob/nam20485/.github/CLA.md` (naming the branch keeps it stable regardless of the default branch).
7. Options: enable **"Automatic re-checks"** and **"Require CLA check to pass"**-style behavior (CLA Assistant posts a `cla-assistant` status check); leave "contributors auto-sign" OFF so humans consciously accept.
8. Save. Open any existing PR — the bot should post "Contributor License Agreement is not yet signed" with a sign link; signing yours (as owner) is a good smoke test.
9. Optional hard gate: branch protection on `nam20485`/`development` requiring the `cla-assistant` check — do this only after PR #610 merges, so the open PR isn't blocked mid-flight.

### A4. Carve out wen2so/Jandody contributions (chain-of-title cleanup) ☐
**Parent:** guide 1.3(b) · **Decided 2026-11-08: remove/replace rather than chase retroactive releases.** The public AGPL history keeps their commits (fine); commercial builds must derive from a tree with zero un-relicensed third-party code.

**Complete inventory (verified 2026-11-08):**

| Author | Commit(s) | Change | Durable footprint in current tree |
|---|---|---|---|
| wen2so | `212429c` | MatrixFile color support | `OdbDesignLib/FileModel/Design/MatrixFile.cpp/.h` color members, `protoc/color.proto`, CMakeLists entry, regenerated `matrixfile.pb.*` fields |
| wen2so | `fbe5b3b` | PolygonPart::type enum conversion fix | 2 lines in `FileModel/Design/ContourPolygon.cpp` |
| Jandody | `a1ceda3`/`59ce5a2` (same change, re-landed) | Entire DRILL/tools-file parser | `FileModel/Design/ToolsFile.cpp/.h`, `LayerDirectory.cpp/.h` wiring, `MatrixFile.cpp` drill-related changes, `protoc/toolsfile.proto`, CMakeLists entries, tests |
| Jandody | `231164a`/`ad1d82c` (same change, re-landed) | ToolsFile naming consistency | Subsumed by the ToolsFile removal above |

(Bots — dependabot/StepSecurity — and `copilot-swe-agent` hold no independent copyright interest.)

Steps:

1. Cut branch `nam/contributor-carveout` from `nam20485` (after PR #610 merges, to avoid conflicts).
2. Impact scan: `grep -rn "ToolsFile\|toolsfile" --include=*.h --include=*.cpp --include=*.proto` + check server routes/tests referencing tools data (`ToolsFileTests`, LayerDirectory wiring, any REST/gRPC surface).
3. Remove the ToolsFile feature: delete `ToolsFile.cpp/.h` + `toolsfile.proto`, un-hook `LayerDirectory` wiring, drop the color members from `MatrixFile` (`212429c`) unless you prefer to reimplement color (it's ~14 lines — reimplementation by you is also clean chain-of-title), revert the 2 `ContourPolygon.cpp` lines (or reimplement the cast — verify against the spec which enum mapping is correct).
4. Delete/adjust the affected tests; full `ctest` green on both Linux presets.
5. PR → `nam20485` (merge commit). Note in the PR body: *not* a license issue for the public repo — cleanup for commercial chain-of-title.
6. Bump the `upstream/OdbDesign` submodule in `OdbDesign-Commercial` to the merge commit; rebuild + rerun the LicenseGuard/commercial gauntlet.
7. Append the closure (date, commits removed, PR link) to the provenance memo (A2).
8. Going forward, CLA (A3) covers new contributions.

### A5. Build the KYC / export-screening gate ☐
**Parent:** guide 1.7/4.2

1. Draft the one-page intake questionnaire (B-side of 4.2): legal entity + country; deployment model (OEM/desktop/server); platforms; expected volume; technical contact; procurement process; entity that will sign.
2. Screening procedure: before issuing any quote/license, check the counterparty + principals against the BIS Consolidated Screening List (https://www.bis.doc.gov/index.php/policy-guidance/consolidated-screening-list) and EU/sanctions lists; save a dated PDF/screenshot of results to the sales ledger.
3. Record the check in the delivery runbook as a required checklist step (it already has a per-delivery checklist in the private repo README).

### A6. Mutual NDA + Evaluation Agreement (western) + bilingual China NNN ☐
**Parent:** guide 4.3 (scope restored 2026-11-08 with the licensed-eval decision)

1. Pick a mutual-NDA template (counsel, or a reputable free template such as the IPLC/NLTPA-style mutual NDA) — keep it mutual, 2–3 years.
2. Add a short **Evaluation Agreement** rider matching the enforced technical terms (guide 2.4): 30-day time limit, watermarked binaries, loopback-only for server sidecars, no benchmarking/publication, no production use, return/delete at end.
3. For PRC counterparties (e.g. LinBoard3D): use a bilingual English/中文 **NNN** (Non-Disclosure / Non-Use / Non-Circumvention) — NDAs alone are weak for PRC enforcement; NNN + governing-law choice is the standard mitigation. Have China-experienced counsel review.
4. Store executed copies in the sales ledger.
5. When a prospect requests an evaluation: issue via the private repo runbook — `tools/make-license.sh` (30-day expiry, `loopback_only` for sidecars) + the packaging workflow with evaluation watermarks; record license-id + customer + expiry in the ledger.

### A7. Master Commercial License Agreement + Schedules A/B ☐
**Parent:** guide 4.4

1. Engage counsel to draft the MCLA once B1/B2 (model + pricing) are decided — the schedules need both.
2. Schedule A: OEM machine bundling (grant, per-machine/subscription metrics, "runtime rights perpetual for machines shipped during term", watermark/audit clause, ODB++ disclaimer, EAR clause).
3. Schedule B: Enterprise server + seats (instance/seat metrics, support tier reference).
4. Loop in the checklist: the Siemens ODB++ disclaimer (guide 1.6) and export clause (1.7) are mandatory clauses.

---

## Section B — Commercial decisions (yours; needed for the quote, not today's reply)

### B1. Choose the licensing model(s) ☐  (parent 2.1/2.2)
1. Re-read guide §2 Models A (annual OEM tiers) / B (base + per-machine packs) / C (server + seats).
2. Decide the **default Keiron offer** (recommendation: present A *and* B, let volume decide) and the **default enterprise offer** (C).
3. Record decisions in this checklist + guide 2.1/2.2 statuses; the license-payload fields already support all of them.

### B2. Set prices ☐  (parent 2.3)
1. Using guide §2 ranges, set: OEM tier prices + machine-pack prices; enterprise server + seat prices; support tier add-ons (Silver/Gold-style); SOW hourly rate.
2. Sanity floor: minimum annual revenue vs your support-hour cost ($/hr × caps) + tooling.
3. Decide renewal/rules (guide 2.3) and 30-day payment terms.
4. Update the one-pager (C6) once set.

### B3. Billing/tax stack ☐  (parent 5.1)
1. Decide: direct PO + wire/Stripe invoice for ≥$10k (recommended for Keiron-class deals); Polar.sh only if you later add self-serve tiers.
2. If invoicing from The Intel Agency: confirm the entity's bank/tax setup with your accountant (EU reverse-charge VAT handling on the invoice).

---

## Section C — Technical follow-ups (small, sequenced)

### C1. Merge PR #610 → bump overlay submodule ◐
**Parent:** guide Step 3 · PR #610 all-green, MERGEABLE.
1. `gh pr merge 610 --merge` (merge commit only).
2. In `OdbDesign-Commercial`: `cd upstream/OdbDesign && git fetch && git checkout <merge-commit>`; commit + push the submodule bump; rebuild + rerun the commercial gauntlet.

### C2. GPG secrets + packaging dry run ☐  (parent 3.1)
1. Private repo → Settings → Secrets → add `GPG_PRIVATE_KEY` + `GPG_PRIVATE_KEY_PASSPHRASE` (same key material as the public repo's release workflow).
2. Run the **Package Commercial Build** workflow with test watermarks; verify artifacts + signatures; deliver to yourself as a dry run.

### C3. SBOM per delivery ☐  (parent 3.5)
1. In the packaging workflow, download the latest `spdx-sbom` artifact from the public repo's SBOM workflow (or run `syft` on the archive) and drop it into `out/dist/<name>/`.

### C4. `OdbDesignConfig.cmake` + install presets ☐  (parent 3.4)
1. Add a minimal `OdbDesignConfig.cmake` (imported targets `OdbDesign::OdbDesign`, `Utils::Utils`) + `install(EXPORT)`; add `installPresets` to `CMakePresets.json`.
2. Extend the out-of-tree consumer check to consume via `find_package(OdbDesign)` in CI or locally.

### C5. COMMERCIAL-LICENSE.md evaluation wording ✓ (parent 2.4)
Settled on the PR #610 branch (2026-11-08, same-day amendment): evaluation described as a time-limited **licensed** evaluation build under an evaluation agreement (watermarked; loopback-only for sidecars) with the public-domain sample archive for validation — not the source-POC path.

### C6. Keiron commercial overview one-pager/PDF ☐  (parent 4.1 — after B1/B2)
1. Once pricing is set, generate a 2-page overview from `COMMERCIAL-LICENSE.md` + guide §2 (models, process, support tiers, contact).
2. Export to PDF; attach to the follow-up after the scoping call.

---

## Section D — Support & operations (after B2)

### D1. Support-tier contracts + rate card ☐  (parent 5.2)
1. Turn the tier structure (standard included / premium response tiers with hour caps / SOW rate) into contract-ready language in the MCLA schedules (A7).
2. Define the escalation path (email → issue tracker → phone for P1) and your response calendar (US Pacific; note CET overlap commitments for EU customers like Keiron).
