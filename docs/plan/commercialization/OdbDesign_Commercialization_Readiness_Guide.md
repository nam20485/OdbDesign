# **OdbDesign Commercialization & License Grant Readiness Guide**

*Two-Pass Decision Playbook: High-Level Readiness Checklist, Concrete Options & Feasibility Analysis for OdbDesign (C++17 Shared Library & REST/gRPC Server), and AI Commercialization Skills, Agents & Platforms — Prepared November 6, 2026*

> **EXECUTION STATUS UPDATE — November 7, 2026.** The repo-side workstreams of this guide (Steps 3–4 of the execution sequence) were implemented and verified. Each Pass 1 checklist item below now carries an inline **Status (2026-11-07)** line, and the full per-domain record — decisions, implementation details, verification evidence, and next steps for deferred items — is in the **Execution Status Update** section at the end of this document, before the references. Key artifacts: public-repo PR **#610** (`nam/commercial-readiness` → `nam20485`, all checks green) and the **private** overlay repository `nam20485/OdbDesign-Commercial`. Companion engineering plan: `commercial-readiness-execution-plan.md` (same directory).

Executive Context: Two Active Inbound Commercial Opportunities dictate Dual Track Readiness 

Transitioning nam20485/OdbDesign (snapshot commit 59ed1cde) from an open-source GNU Affero General Public License v3.0 (AGPL-3.0) repository into a commercial-ready B2B software product requires resolving legal, architectural, packaging, and operational decisions before issuing a binding quote or license grant. Your workspace currently holds two live commercial licensing inquiries with starkly different risk profiles and integration models: 

Inquiry 1 — European Industrial SMT Hardware OEM (Keiron Printing Technologies BV): On November 5, 2026, Software Engineer Ruben van Oers (r.vanoers@keirontechnologies.com, CC: Frank Ophelders and Martin de Liefde) reached out regarding OdbDesign (Keiron OdbDesign Email Thread). Headquartered at High Tech Campus 29 in Eindhoven, Netherlands, and backed by a €20.7M Series A funding round in July 2026, Keiron builds the HF2 LiFT Printer (Laser-Induced Forward Transfer digital stencil-free solder paste printer). Keiron wants to integrate the OdbDesign library into their closed-source Job Preparation Software shipped with each HF2 machine and asked three specific questions: (1) what the process looks like to obtain a closed source/commercial license, (2) which license models are available, and (3) what future support options exist. 

Inquiry 2 — Unverified Cross-Border Desktop/Server Prospect (LinBoard3D Studio): On October 27, 2026, Lin Qijin (林啟进, 3270963097@qq.com, Jiaxing, Zhejiang, China) requested a commercial license to bundle OdbDesignServer and its DLLs as a local loopback REST sidecar inside a closed-source Windows PCB viewer/DFM installer, while also asking about centralized enterprise on-premise server deployment, source code escrow, SBOMs, and 2–3 representative ODB++ .tgz validation archives (LinBoard3D Email Thread). Your existing forensic analysis in OdbDesign Inquiry Entity Investigation and policy decisions in LinBoard3D Studio’s OdbDesign Licensing Inquiry Stance establish strict guardrails against enterprise proxy under licensing and proprietary test fixture leakage. 

This guide is structured in three parts: Pass 1 provides a high-level generic checklist of every milestone required to declare commercial readiness; Pass 2 provides a concrete feasibility analysis, option matrix, pros/cons, and specific recommendation for OdbDesign across each milestone; and Part 3 catalogs verified AI skills, agents, and platforms that automate software commercialization, positioning, and pricing. 

Pass 1: High-Level Generic Commercialization & License-Grant Readiness Checklist 

Before selecting specific terms or writing code, use this first-pass checklist to see every workstream that must be investigated, decided, or implemented to declare full readiness to create, present, and grant commercial licenses.

OdbDesign Commercialization & License Grant Readiness Guide Page 1 of 12   
ID Readiness Domain & Item Action Type What Must Be Resolved to Declare Readiness 

1.1 Licensor Corporate Entity & Liability Shield   
Investigate & Decide   
Select the legal entity that will sign commercial contracts, hold commercial licensing rights, and shield personal assets from warranty or indemnity claims.

> **Status (2026-11-07): Deferred (user decision).** Recommendation stands (The Intel Agency now → dedicated WA LLC at $50k ARR). Nothing executed yet; must be resolved before the first binding quote or grant. Next: confirm entity + execute the 1-page internal IP license. 

1.2 Employer CIIAA & Invention Separation

> **Status (2026-11-07): Deferred (ongoing discipline).** No repository action possible or taken; the conduct rule (personal hardware/time only, zero employer resources) remains a precondition on every future commercial artifact.

1.3 Copyright Chain-of-Title & Contributor Policy

> **Status (2026-11-07): In progress.** CLA text landed in PR #610 (`.github/CLA.md` — Apache-style ICLA + CCLA with the explicit commercial *relicensing* grant, §1.2(2)) plus a `docs/CONTRIBUTING.md` update; CODEOWNERS re-verified as 100% `@nam20485`. **Remaining:** CLA Assistant enablement (maintainer browser step — store signatures in a side repo, point it at `.github/CLA.md`) before accepting any external PR.
Verify & Document Verify statutory carve-out compliance under WA RCW 49.44.140 so no employer equipment, time, or networks 

touch the commercialized codebase. 

Audit & Decide Confirm 100% copyright ownership across git commit history to legally relicense AGPL-3.0 code, and establish 

a Contributor License Agreement (CLA) policy for future 

PRs. 

1.4 Dual-Licensing Legal Framework

> **Status (2026-11-07): Complete (public-repo side).** Option A retained (AGPL-3.0 default + paid commercial). `COMMERCIAL-LICENSE.md` added at repo root — deliberately **price-free** (grants/non-grants, 4-step onboarding, evaluation terms, support-tier structure) — and linked from both `docs/README.md` license sections and `docs/one_pager.md`. Pricing remains a live business decision and is intentionally not published.
Investigate & Decide   
Decide how the public open-source license (AGPL-3.0 or source-available alternatives) coexists with paid closed source commercial license grants. 

1.5 Third-Party Dependency License Compliance 

1.6 Format & Trademark IP Disclaimers (ODB++) 

1.7 Export Control (EAR) & Counterparty KYC   
Audit & Package Verify that every linked library in vcpkg.json permits closed-source binary redistribution and compile a 

compliant THIRD\_PARTY\_NOTICES.txt file.

> **Status (2026-11-07): Complete.** `THIRD_PARTY_NOTICES.txt` committed at repo root, generated by the new `scripts/generate-third-party-notices.sh` from the vcpkg `copyright` files of every installed port (16 ports including all five direct deps — libarchive, zlib, Crow, Protobuf, gRPC, all permissive). Decision: exact versions are referenced via `vcpkg.json` pins + the SPDX SBOM rather than embedded in the notices file; the private packager regenerates the full transitive set per delivery.

Draft & Enforce Enforce a strict contractual boundary disclaiming any transfer of Siemens EDA's non-transferable ODB++   
Solutions Alliance format IP or trademarks.

> **Status (2026-11-07): Complete in public collateral; contract clause pending.** `COMMERCIAL-LICENSE.md` carries a dedicated "What the commercial license does NOT grant" section (no ODB++ format/trademark rights; licensees register directly with the ODB++ Solutions Alliance). The binding contract language belongs in the Master License Agreement schedules (item 4.4, deferred to legal drafting).

Establish & Enforce Establish a screening check against the BIS Consolidated Screening List and define a KYC 

onboarding gate for foreign buyers.

> **Status (2026-11-07): Partially complete.** `COMMERCIAL-LICENSE.md` gained a compliance-notes section (EAR99-classification belief + counterparty screening statement). The operational KYC gate — questionnaire (item 4.2) plus a defined BIS/CSL screening step in the onboarding runbook — is **not yet built**. Next: draft the intake questionnaire; add the screening checkpoint to the delivery runbook.

2.1 Commercial Product SKU & Boundary Definition

> **Status (2026-11-07): Technical boundaries complete; commercial SKUs deferred (user).** The three artifacts now exist as distinct, packageable units: the C++ SDK (parser-only `OdbDesignParser.h` surface + install tree), the full server (`OdbDesignServerLib`-linked), and the loopback sidecar (commercial server under a `loopback_only` license). Naming a SKU and its price is still open (items 2.2/2.3).

2.2 Commercial Licensing Model Selection

> **Status (2026-11-07): Deferred (user decision).** Models A/B (OEM) and C/D (enterprise/eval) remain the menu. Implementation note: the license payload's `edition` + `features` + `loopback_only` fields can express any of them without rework — the enforcement side is model-agnostic.

2.3 Pricing Metrics, Tiers & Volume Thresholds

> **Status (2026-11-07): Deferred (user decision).** No prices published anywhere by design (`COMMERCIAL-LICENSE.md` is deliberately price-free). Guide §2 tiers remain the working proposal.

2.4 Evaluation / Trial Licensing Policy

> **Status (2026-11-07): Technical enforcement complete; paperwork deferred.** 30-day time-locked evaluation is cryptographically enforced (`expires_at` in the Ed25519-signed `license.json`), `loopback_only: true` forces 127.0.0.1 on **both** REST and gRPC (proven to override even `--bind 0.0.0.0`), and eval binaries are watermarked + stripped. The Evaluation Agreement itself is item 4.3 (legal, not drafted).

2.5 Source-Code Access & Escrow Policy

> **Status (2026-11-07): Deferred.** Binary-only is the implemented default. The guide's position (beneficiary-paid Codekeeper/Escode escrow; Iron Mountain exited 2021) stands — select an agent when a buyer first requests continuity assurance.
Investigate & Decide 

Investigate & Decide 

Investigate & Decide 

Investigate & Decide 

Investigate & Decide   
Define the exact commercial product SKUs offered (Native C++ Shared Library SDK OdbDesign.dll/.so vs. Local Loopback Sidecar Server vs. Enterprise Network Server). 

Choose the primary licensing structures for hardware OEMs (Per-Machine Royalty vs. Annual Unlimited OEM Subscription) and enterprise software vendors (Server \+ Per-Seat Client vs. Perpetual). 

Establish concrete price ranges, minimum seat/machine commitments, and annual renewal rules for each SKU and buyer tier. 

Decide whether and how to offer 30–90 day evaluation binaries, what technical restrictions apply during trials, and whether an evaluation deposit or NNN/NDA is required first. 

Decide whether commercial licenses are strictly binary only, when third-party source escrow is permitted, and who pays escrow setup/maintenance fees. 

3.1 Private Commercial Build Overlay & CI/CD

> **Status (2026-11-07): Complete.** Private repository **`nam20485/OdbDesign-Commercial`** created (visibility verified PRIVATE on GitHub) — *not* a branch, because every ref of a public repo is fetchable (see errata). Contents: upstream git submodule pinned to the `nam20485` branch, CMake superbuild (`add_subdirectory` + upstream install rules), mirrored vcpkg manifest/baseline (plus `openssl`, `nlohmann-json`), manual-dispatch per-customer packaging workflow with GitHub-Actions vcpkg binary caching, and a delivery runbook README. Local builds seed `vcpkg_installed` from the upstream build tree to skip the ~45-min dependency rebuild.
Architect & Build Set up a private commercial build repository/branch that pulls from upstream nam20485/OdbDesign and compiles   
commercial binaries without exposing license-check 

source publicly. 

3.2 Technical License Enforcement Mechanism

> **Status (2026-11-07): Complete (Option 2, as recommended).** `LicenseGuard` — a ~300-line C++17/OpenSSL-EVP static library in the private overlay — verifies a `license.json` document: Ed25519 signature over the exact bytes of a canonical (sorted-key, compact) JSON payload; then expiry (`issued_at`/`expires_at`, RFC3339 UTC), required fields, and product match. **13/13 unit tests** (valid / expired / tampered / wrong key / missing field / bad base64 / product mismatch / date parsing, plus a committed dev-key fixture round-trip). `OdbDesignCommercialServer` (overlay-owned `main`) verifies **before constructing the server app**, so a bad license exits non-zero with zero sockets opened — proven live. Option 3 (Keygen.sh/LicenseSpring) deliberately deferred: same Ed25519 scheme, adoptable later without rework. **The public repo contains zero enforcement code** — an AGPL user could strip it anyway; dual-licensing relies on contract + copyright, with enforcement as a convenience layer for evaluation builds.
Investigate & Decide   
Select and implement the runtime license verification mechanism (Honor-System Watermark vs. In-House OpenSSL Ed25519 license.json vs. Commercial C++ Licensing SDK). 

3.3 Binary Watermarking & Symbol Stripping

> **Status (2026-11-07): Complete (Option 1, as recommended).** CMake generates `Watermark.cpp` at build time embedding `ODBDESIGN_CUSTOMER_ID` / `ODBDESIGN_LICENSE_ID` / build timestamp / build type (prefixed `ODBDESIGN-COMMERCIAL:…`, visible via `strings`); release builds POST_BUILD `strip --strip-unneeded` on Linux (Debug never stripped; Windows strips via linker settings), and the packaging workflow archives a private symbols copy for crash debugging. `ODBDESIGN_WATERMARK_SDK_DLL=ON` additionally links the provenance object **into** the upstream `OdbDesign.dll`/`libOdbDesign.so` for OEM production SDK deliveries — no public-repo source changes (the overlay mutates the target from outside). Verified live: watermark strings present, `nm` reports "no symbols", `file` reports "stripped".  
Implement & Verify Configure CMake release builds to strip debug symbols (.pdb / DWARF) and embed a customer-specific

OdbDesign Commercialization & License Grant Readiness Guide Page 2 of 12   
ID Readiness Domain & Item Action Type What Must Be Resolved to Declare Readiness 

provenance UUID watermark into every distributed 

binary. 

3.4 SDK Header/ABI & Container Packaging

> **Status (2026-11-07): Complete (install basis).** The repo's first-ever `install()` rules (PR #610): targets (`OdbDesign` SHARED, `Utils`, `OdbDesignServer` + new `OdbDesignServerLib`) with RUNTIME/LIBRARY/ARCHIVE destinations; public headers preserving structure; the 26 generated `.pb.h` installed alongside (public headers include them by bare filename); `INSTALL_GTEST OFF` keeps googletest out of the SDK tree. New parser-only umbrella **`OdbDesignParser.h`** (FileModel + ProductModel + errors; no `App/`, no Crow) is the supported customer entry — proven by a PCH-exempt smoke test and by a 20-line **out-of-tree consumer** that compiles/links/runs against the installed tree alone (flags: `-I include/OdbDesign -I include/Utils -I <vcpkg>/include`, libs `-lOdbDesign -lUtils -lprotobuf`). The packaging workflow bundles third-party headers (`google/`, `absl/`, `crow.h` + `crow/`) and runtime shared libraries into the per-customer archive. **Follow-ups:** `OdbDesignConfig.cmake` for `find_package(OdbDesign)` consumption; first workflow dry-run (blocked only on GPG secrets in the private repo).

3.5 Automated SBOM & Compliance Bundle

> **Status (2026-11-07): In progress.** `sbom-generate-submit.yml` now also uploads the SPDX document as a workflow artifact (30-day retention) — PR #610 — so deliveries can bundle it without re-running generation. **Remaining:** pull that artifact (or run `syft` against the archive) inside the private packaging workflow so every per-customer zip ships with its SBOM.

3.6 Validation Sample Fixture Governance

> **Status (2026-11-07): Complete — audit found the governance already sound; nothing to fix.** Verified against the live repos: public `OdbDesignTestData` contains only `designodb_rigidflex*` variants + IPC-2581 files; the proprietary designs (Panel-g7162, 200-40628, 350-41017, Turbot) exist only in local git-ignored directories and were never published. `sample_design.tgz` (public-domain) is **already** tracked and public in `OdbDesignTestData` — so external evaluators have a known-good archive today, eval bundles include it via the packager, and CI gains test coverage for free (the parser-only smoke test runs against it).

4.1 Commercial Licensing Overview / One-Pager

> **Status (2026-11-07): In progress.** The repository-side overview is done — `COMMERCIAL-LICENSE.md` (process, grants/non-grants incl. the Siemens ODB++ disclaimer, evaluation terms, support-tier structure, price-free) plus updated README/one-pager links. The 2-page customer-facing **PDF/deck for Keiron** was deliberately excluded from the approved scope this round. Next: produce it (content can be derived almost entirely from `COMMERCIAL-LICENSE.md` + this guide's §2 pricing tables once pricing is decided).
Complete & Verify Create clean CMake install() rules that package public headers, generated Protobuf headers, shared 

libraries, and runtime vcpkg DLLs/SOs into a turnkey 

customer SDK archive. 

Complete & Verify Package the SPDX/CycloneDX SBOM from .github/workflows/sbom-generate-submit.yml 

alongside THIRD\_PARTY\_NOTICES.txt and OpenSSF 

Scorecard evidence. 

Audit & Restrict Isolate the public-domain sample\_design archive for external evaluation and strictly lock down proprietary 

test fixtures (Panel-g7162, 200-40628, 350-41017). 

Create & Finalize Produce a clean B2B licensing overview document and presentation deck explaining the integration options, 

licensing process, SKU tiers, and support options. 

4.2 Pre-Evaluation KYC & Scope Questionnaire

> **Status (2026-11-07): Deferred — not drafted.** The content outline already exists (§ here: deployment model OEM vs. desktop vs. server, platforms, expected volume, corporate identity). Next: turn into a one-page intake form; it also feeds the item-1.7 screening gate.  
Create & 

Standardize   
Draft a concise technical/commercial intake questionnaire to capture target deployment (OEM machine vs. desktop vs. server), expected unit volume, and corporate identity. 

4.3 Evaluation Agreement & Bilingual China NNN

> **Status (2026-11-07): Deferred — legal drafting not started.** Needed before shipping the first evaluation binary to a third party: Western Mutual NDA + 30-day Evaluation Agreement (Keiron path) and bilingual China NNN (LinBoard3D path). Technical eval restrictions are already enforced (see 2.4); the paper must match them (30 days, loopback-only, watermark acknowledgment, no benchmarking/disclosure).

4.4 Master Commercial License Grant &
Schedules

> **Status (2026-11-07): Deferred — legal drafting not started.** MCLA + Schedule A (OEM Machine Bundling) + Schedule B (Enterprise Server + Seats), embedding the Siemens ODB++ Alliance disclaimer (1.6) and EAR/export clause (1.7). Recommended route: counsel-drafted from the Apache-2.0 grant structure used in `.github/CLA.md` as a familiar reference point.  
Create & Finalize Prepare a standard Western Mutual NDA \+ 30-Day Evaluation Agreement (for EU/US OEMs like Keiron) and   
a Bilingual China NNN Agreement (for PRC entities like 

LinBoard3D). 

Create & Finalize Draft the formal Master Commercial Software License Agreement with modular Order Schedules (Schedule A:   
OEM Machine Bundling; Schedule B: Enterprise Server \+ 

Seats). 

5.1 Billing, Tax & Payment Infrastructure

> **Status (2026-11-07): Deferred.** Hybrid recommendation stands (PO + wire/Stripe ≥ $10k; Polar.sh for self-serve later). No repository dependency blocks this; decide before the first invoice.

5.2 Fulfillment, Support SLA & Consulting Tiers

> **Status (2026-11-07): Partially complete.** The fulfillment **mechanism** is built (manual-dispatch packaging workflow → watermarked, signed, private artifact → runbook delivery checklist with per-delivery verification steps). The support-tier **structure** is documented price-free in `COMMERCIAL-LICENSE.md`. The SLA contracts themselves (response-time tiers with hour caps, SOW rate card) are not authored — pending the pricing decision (2.3) and legal drafting (4.4).
Investigate & Decide 

Investigate & Decide   
Select the payment and tax compliance stack (Merchant of Record such as Polar.sh or Paddle vs. direct corporate wire / Stripe invoicing). 

Define the private binary/key delivery mechanism, response-time-only SLA tiers (respecting your ≤35 hr/wk capacity limit), and hourly/SOW custom engineering rates. 

Pass 2: Concrete Options, OdbDesign Feasibility Analysis, Pros/Cons & Recommendations 

This second pass evaluates the concrete options for each readiness domain from Pass 1, grounded directly in your OdbDesign repository (\`59ed1cde\`), your existing stance documents in Google Drive, and the technical requirements of both Keiron Printing Technologies and LinBoard3D Studio. 

Domain 1: Legal, IP & Corporate Governance Readiness (Items 1.1 – 1.7) 

OdbDesign Current State & Audit Findings: A direct inspection of your repository snapshot (\`59ed1cde\`) confirms that .github/CODEOWNERS assigns 100% ownership to @nam20485, and source files (such as OdbDesignLib/OdbDesign.h and OdbDesignServer/main.cpp) do not contain third-party AGPL copyright headers. In vcpkg.json, all five direct dependencies are permissively licensed: libarchive 3.8.7 (BSD-2-Clause), zlib 1.3.2\#2 (Zlib), Crow 1.3.3 (BSD-3-Clause), Protobuf 6.33.4\#2 (BSD-3-Clause), and gRPC 1.81.1 (Apache 2.0). However, docs/CONTRIBUTING.md currently accepts pull requests against development without requiring

OdbDesign Commercialization & License Grant Readiness Guide Page 3 of 12   
a Contributor License Agreement (CLA), and you signed an employment offer and CIIAA with Cincinnatus LLC (Cincinnatus CIIAA PDF) effective June 26, 2026\. 

Decision Area Concrete Options Pros & Cons for OdbDesign Recommendation & Why 

1.1 Licensor 

Corporate Entity (WA Chapter 25.15 RCW) 

1.2 Employer CIIAA Separation (WA RCW 49.44.140) 

1.3 Contributor Policy & Chain of Title 

1.4 Public vs. 

Commercial 

License Pairing   
Option A: Sign personally as Nathan Miller.   
Option B: License through your existing entity, The Intel Agency (intel-agency.com). Option C: Form a dedicated WA LLC (OdbDesign LLC, \$200 WA online filing \+ \$70/yr report). 

Option A: Informal separation. Option B: Strict statutory wall \+ written provenance log under RCW 49.44.140 and RCW 49.44.150. 

Option A: Keep current 

CONTRIBUTING.md (no 

CLA/DCO). 

Option B: Add Developer Certificate of Origin (DCO v1.1). Option C: Add an Apache-style Individual/Corporate CLA with explicit commercial relicensing rights via CLA Assistant. 

Option A: Keep public repo under AGPL-3.0 \+ offer paid Commercial OEM/Enterprise License. 

Option B: Switch public repo or new modules to Functional Source License (FSL-1.1) or PolyForm Noncommercial 1.0.0.   
Opt A: Zero setup cost, but exposes personal assets to unlimited B2B contract/IP liability. 

Opt B: Immediate readiness; already hosts intel 

agency/odbdesign-3d client-prototype. 

Opt C: Cleanest ring-fencing of OdbDesign IP from other consulting/agent ventures. 

Opt A: Vulnerable if a git commit timestamp or email header overlaps with employer hours/devices.   
Opt B: Preserves your 

statutory immunity under Washington law. 

Opt A/B: A DCO only certifies contributions under the repo's current AGPL-3.0 license (Open Source Guide)—one external PR without a 

relicensing grant would taint your ability to sell closed source commercial licenses. Opt C: Preserves 100% commercial relicensing authority automatically on GitHub. 

Opt A: Already working—both Keiron and LinBoard3D explicitly contacted you because they saw AGPL-3.0 in your README and knew they needed a commercial license. Opt B: PolyForm   
Noncommercial blocks even internal corporate evaluation without a license; FSL-1.1 allows free internal corporate use, which weakens your leverage for internal factory tools.   
Recommend Option B 

immediately (The Intel Agency), transitioning to Option C if ARR exceeds \$50k. 

Why: Never sign an OEM or enterprise contract personally. Using The Intel Agency lets you respond to Keiron today under a corporate liability shield (RCW 25.15.126) once you execute a simple 1-page internal IP license from yourself to the entity. 

Recommend Option B. 

Why: Under WA RCW 

49.44.140, an employer CIIAA cannot claim an invention developed entirely on your own time using zero employer equipment/supplies/facilities. Conduct all OdbDesign commits and 

Keiron/LinBoard3D 

communications strictly on personal hardware and non employer hours. 

Recommend Option C. Why: Dual-licensing requires that the Licensor hold either sole copyright or an explicit commercial relicensing grant for every line of code (License Cookbook). Adding CLA Assistant to 

nam20485/OdbDesign takes 15 minutes and protects your commercial chain of title permanently. 

Recommend Option A (Keep AGPL-3.0 on Public Repo \+ Paid Commercial License). Why: Your current AGPL-3.0 strategy is already generating qualified inbound leads from both European OEMs (Keiron) and Asian DFM vendors (LinBoard3D). Add a 

COMMERCIAL-LICENSE.md file to the repo root to formalize the dual-license offer.

OdbDesign Commercialization & License Grant Readiness Guide Page 4 of 12   
Decision Area Concrete Options Pros & Cons for OdbDesign Recommendation & Why 

1.6 Format IP & Export Controls   
Option A: Silent contract. Option B: Explicit Siemens ODB++ Solutions Alliance non sublicense clause \+ BIS EAR99 / Part 744 export compliance clause.   
Opt A: Creates severe liability if a buyer assumes your license grants Siemens ODB++ trademark rights. Opt B: Completely insulates you from third-party format IP claims and export diversion.   
Recommend Option B. 

Why: Siemens EDA's ODB++ Solutions Alliance agreement is free but non-transferable,   
and Ucamco owns Gerber, not ODB++. Every contract must state that the Licensee obtains copyright rights solely to your OdbDesign software and must register directly with Siemens EDA's Alliance for ODB++ format/trademark use. 

Domain 2: Commercial Product SKUs, Licensing Models & Pricing Readiness (Items 2.1 – 2.5) 

OdbDesign Current State & Feasibility Assessment: Your repository builds two distinct commercializable artifacts via CMake: (1) OdbDesign (OdbDesign.dll on Windows / libOdbDesign.so on Linux), the C++17 shared library exposing FileModel, ProductModel, and Protobuf serialization; and (2) OdbDesignServer, the Crow REST \+ gRPC server executable (recently upgraded in PR \#606 with server-owned Connectivity and PR \#609 with chunked GetDesignStream). Notice how your two active prospects map to different artifacts: Keiron Printing Technologies explicitly asked to integrate "the library... into an application used for job preparation, which is included with the machine" (Keiron Email), whereas LinBoard3D Studio asked to bundle OdbDesignServer as a local loopback sidecar or centralized enterprise server (LinBoard3D Email). Meanwhile, in LinBoard3D Studio’s OdbDesign Licensing Inquiry Stance, you established a core principle: never offer a cheap "personal" license that can be exploited in an enterprise or multi-seat environment. 

Commercial Option   
Structure & Target SKU Pros & Cons for OdbDesign Recommendation & Why 

Model A: Annual OEM Machine Bundling   
Subscription 

(Tiered Flat Fee) 

Model B: Base OEM Development Subscription \+ Per-Machine 

Runtime Packs   
SKU: OdbDesign-OEM (C++ Shared Library SDK \+ Local Sidecar Binary).   
Pricing: 100% upfront annual fee: 

• Tier 1 (≤25 machines/yr): \$15,000/yr   
• Tier 2 (≤100 machines/yr): \$28,000/yr 

• Tier 3 (Unlimited OEM): \$45,000/yr   
Includes up to 5 internal R\&D developer seats and all v1.x updates/CVE patches. Runtime rights on machines shipped during an active term are perpetual for the end buyer. 

SKU: OdbDesign-OEM. Pricing: \$7,500/yr Base R\&D SDK Subscription (up to 5 R\&D seats) \+ Prepaid 

Runtime Royalty Packs (e.g., \$7,500 for a pack of 25 machine licenses \= 

\$300/machine; or \$12,500 for 50 machines \=   
\$250/machine).   
Pros: Zero per-machine royalty counting or air-gapped 

telemetry friction; 100% upfront cash flow; gives a hardware OEM like Keiron predictable software COGS while granting perpetual runtime rights to their factory customers for machines shipped during the active subscription. 

Cons: Requires clear contract language that new machine shipments stop if the annual OEM subscription is not 

renewed. 

Pros: Lower Year-1 entry price if an OEM is still in low-volume NPI/pilot production (e.g., shipping 10–15 HF2 systems in Year 1), while scaling revenue automatically as their machine fleet grows. 

Cons: Requires an annual self certification royalty report from the OEM's finance officer.   
Recommend as Primary Offer for Keiron Printing 

Technologies (paired with Model B as an alternative). Why: Industrial SMT equipment OEMs building high-ASP capital equipment (Keiron HF2) strongly prefer predictable annual tiers or per-machine packs where their end 

customers' factory machines never time-bomb in production. 

Recommend presenting alongside Model A in Keiron's quote. 

Why: Offering both an Annual OEM Unlimited/Tiered   
Subscription (Model A) and a Base SDK \+ Per-Machine Pack Model (Model B) directly answers Ruben van Oers's question ("Which license

OdbDesign Commercialization & License Grant Readiness Guide Page 5 of 12   
Commercial Option   
Structure & Target SKU Pros & Cons for OdbDesign Recommendation & Why 

models are available?") and lets 

Keiron self-select based on 

their 2027 HF2 shipment 

forecast. 

Model C: 

Enterprise Server \+ Per-Client Seat Subscription 

Model D: 

Evaluation & 

Source Escrow Policy   
SKU: OdbDesign 

Enterprise 

(OdbDesignServer 

REST/gRPC \+ Docker/k3s). Pricing: \$12,000/yr per Enterprise Server Instance \+ \$400/seat/yr per client seat (minimum 10-seat block \= \$16,000/yr minimum).   
Optional Loopback Desktop Sidecar Tier: \$12,000/yr Base \+ \$250/endpoint/yr (locked strictly to 127.0.0.1). 

Evaluation: 30-day time locked evaluation binary (loopback-only, 

watermarked). 

Escrow: Binary-only by default; optional third-party escrow via Codekeeper (\$29–\$349/mo) or Escode (formerly NCC Group) paid 100% by Licensee. (Note: Iron Mountain exited 

software escrow in 2021.)   
Pros: Directly enforces your LinBoard3D Stance; eliminates the risk of an unverified party buying a single low-cost license and running a multi-user factory server. 

Cons: High price floor 

intentionally filters out 

hobbyists or under-capitalized indie studios. 

Pros: Lets Keiron validate integration risk-free for 30 days under an Evaluation Agreement; satisfies enterprise continuity requirements without handing over non-AGPL source code directly. 

Cons: Requires compiling a time-locked evaluation build.   
Recommend as Mandatory Model for LinBoard3D Studio & Software ISVs. 

Why: Matches Item \#4 of your LinBoard3D Stance Doc verbatim and protects the enterprise value of your 

gRPC/REST server architecture. 

Recommend 30-Day 

Evaluation Binary \+ 

Beneficiary-Paid 

Codekeeper/Escode Escrow. Why: Protects your IP while offering an institutional-grade solution if Keiron's Series A investors or legal team request continuity assurance. 

Domain 3: Technical & Repository Preparation Readiness (Items 3.1 – 3.6) 

OdbDesign Current State & Feasibility Assessment: Auditing CMakeLists.txt, 

OdbDesignLib/CMakeLists.txt, and OdbDesignServer/CMakeLists.txt reveals four concrete technical preparation items needed before shipping a commercial SDK or server bundle: 

1\. Missing CMake **install()** Target Rules: Currently, OdbDesignLib/CMakeLists.txt builds OdbDesign (SHARED) and generates Protobuf headers into \${CMAKE\_CURRENT\_BINARY\_DIR}, but neither CMakeLists.txt nor OdbDesignLib/CMakeLists.txt defines install(TARGETS OdbDesign ...) or install(DIRECTORY ...) rules to export a clean, standalone include/ \+ lib/ \+ bin/ SDK folder for C++ library customers like Keiron. 

2\. Transitive Header Exposure in **OdbDesign.h**: OdbDesignLib/OdbDesign.h includes App/DesignCache.h and App/OdbAppBase.h, which pull in crow\_win.h and Crow HTTP headers even when a customer only wants to link OdbDesign.dll to parse an ODB++ archive into FileModel / ProductModel. Providing a clean parser-only header entry point (e.g., OdbDesignParser.h or separating OdbDesignCore from HTTP app classes) will drastically simplify Keiron's C++ integration. 

3\. Runtime License Enforcement & Private Commercial Overlay: Currently, 

OdbDesignLib/App/BasicRequestAuthentication.cpp only checks HTTP Basic Auth credentials (ODBDESIGN\_SERVER\_REQUEST\_USERNAME / PASSWORD, defaulting to odb:plusplus). There is no cryptographic license file check or binary watermark. Because nam20485/OdbDesign is public on GitHub, any commercial license check must live in a private commercial repository/branch so users cannot simply recompile without the check.

OdbDesign Commercialization & License Grant Readiness Guide Page 6 of 12   
4\. Existing CI Strengths Ready to Leverage: Your .github/workflows/create-release.yml workflow already builds multi-platform archives (OdbDesign-Windows-x64.zip, OdbDesign-Linux-x64.zip), generates SHA-256 checksums, and signs binaries with GPG (crazy-max/ghaction-import-gpg), and .github/workflows/sbom-generate-submit.yml already generates SPDX SBOMs\! 

Technical Option Implementation Mechanism Feasibility, Pros & Cons Recommendation & Why 

Option 1: 

Watermarked Commercial 

Binary (No Key File) 

Option 2: In 

House Offline **Ed25519** Signed **license.json** (OpenSSL) 

Option 3: Third Party Licensing Engine 

(Keygen.sh or LicenseSpring C++ SDK)   
Private CMake build injects \- DODBDESIGN\_CUSTOMER\_ID="KEIRON 2026-001" and \- 

DODBDESIGN\_LICENSE\_EXPIRY=... into the binary and strips symbols (/DEBUG:NONE on MSVC, strip \-- strip-unneeded on Linux). 

150-line C++17 verifier using OpenSSL EVP\_DigestVerifyInit (already linked in your vcpkg.json tree) verifying a canonical JSON payload \+ Base64 Ed25519 signature (C++ Ed25519 Reference). 

Integrate Keygen.sh offline .lic certificates or link the precompiled LicenseSpring C++ SDK for hardware fingerprinted node-locking and LAN floating seat servers.   
Feasibility: Very High (\~2 hours).   
Pros: Zero runtime failure modes on air-gapped SMT factory machines (Keiron HF2); embeds forensic provenance if a binary leaks. 

Cons: Cannot update expiration dates or tiers without shipping a new binary. 

Feasibility: High (\~1 day). Pros: \$0 SaaS cost; 100% air-gap compatible; 

cryptographically enforces expires\_at (for 30-day evaluations), tier, 

max\_threads, and 

loopback\_only: true (forcing OdbDesignServer to bind 127.0.0.1 instead of 0.0.0.0). 

Cons: Does not node-lock to hardware without 

machine fingerprint 

exchange. 

Feasibility: Medium (\~3–5 days). 

Pros: Turnkey hardware node-locking and on premise floating license server (LicenseSpring Floating Server). 

Cons: Adds an external binary dependency or recurring SaaS fee before closing your first deal.   
Recommend for C++ Shared Library   
(**OdbDesign.dll**/**.so**) OEM Production Releases. Why: Hardware OEMs embedding a DLL inside machine control/job-prep software hate runtime license-file path issues on factory floors. A   
watermarked production DLL \+ contractual audit is standard for embedded C++ SDKs. 

Recommend for All 30-Day Evaluation Binaries & **OdbDesignServer** 

Deployments. 

Why: Solves your \#1 

technical enforcement need for both LinBoard3D Studio and evaluation builds: you can hand out a 30-day evaluation binary or sidecar server where 

license.json 

cryptographically locks the expiry date and forces 127.0.0.1 loopback 

binding. 

Defer until Customer \#3+ or when a buyer requests LAN Floating Seats. 

Why: Since Keygen's offline .lic files use the exact same Ed25519 signature scheme as Option 2 (Keygen C++ Example), building Option 2 now lets you adopt Keygen.sh later with zero architectural rework. 

Domain 4, 5 & 6: Collateral, Billing/Fulfillment & Support/SLA Readiness (Items 4.1 – 6.3) 

OdbDesign Current State & Feasibility Assessment: Your docs/one\_pager.md and docs/README.md explain the open-source architecture well, and docs/README.md already embeds a Google Calendar appointment scheduling link. However, there is no commercial pricing/tier sheet, no standard Commercial License Agreement template, and no formal Support SLA policy that protects your ≤35 hour/week capacity limit.

OdbDesign Commercialization & License Grant Readiness Guide Page 7 of 12   
Readiness Area Concrete Options Pros & Cons for OdbDesign Recommendation & Why 

Billing & Tax 

Compliance Stack (Item 5.1) 

Support SLA & Capacity 

Guardrails (Item 6.2)   
Option A: Direct Corporate Invoicing \+ Wire / Stripe (via The Intel Agency / 

OdbDesign LLC). 

Option B: Developer 

Merchant of Record 

(Polar.sh or Paddle). 

Option A: Unbounded support included in license. Option B: Strict 3-Tier Response-Time SLA \+ Prepaid Hour Caps: 

• Standard Maintenance (Included in active 

subscription): Binary 

updates, SBOMs, CVE patches, 5-business-day email triage. 

• Silver Support 

(+\$6,000/yr): 2-business day P1 response, parser bug prioritization, capped at 15 hrs/yr.   
• Gold OEM SLA 

(+\$18,000/yr): 1-business day P1 response, minor version hotfix backports, capped at 40 hrs/yr. 

• Custom Engineering SOWs: \$250/hr (20-hr prepaid blocks), Licensor retains core IP.   
Opt A: Preferred by enterprise procurement teams (like Keiron in the EU) issuing formal Purchase Orders (POs) and paying via international bank wire/SWIFT or ACH; zero MoR cut. 

Opt B: Polar.sh handles global VAT/sales tax automatically and automates private GitHub repo invites and license key delivery. 

Opt A: High risk of blowing past your ≤35 hr/week work cap if an OEM demands live 

debugging or custom ODB++ vendor extensions. 

Opt B:Directly answers Keiron's Question \#3 ("What are 

possible support options in the future?") with predictable enterprise tiers while bounding your weekly hours.   
Recommend Hybrid: Option A (PO \+ Wire/Stripe Invoice) for Enterprise/OEM contracts ≥\$10k, and Option B (Polar.sh) for self-serve 

developer/evaluation tiers. Why: European hardware OEMs like Keiron buy via B2B PO \+ wire transfer (reverse-charge B2B VAT), whereas Polar.sh automates self-serve binary downloads and VAT if you add a lower-tier SDK subscription later. 

Recommend Option B. 

Why: Structuring SLAs around initial response/triage time (US Pacific business hours) rather than guaranteed resolution windows, combined with annual hour caps (15 hrs Silver / 40 hrs Gold) and a \$250/hr SOW rate for custom features, monetizes your 23 years of C++/Siemens EDA Valor expertise (Master Resume) without endangering your weekly schedule. 

Part 3: Web Research — AI Skills, Agents & Platforms for Product Commercialization 

Just as SEO optimization agents audit and optimize web pages, a rapidly maturing ecosystem of open-source Agent Skills (SKILL.md packages for Claude Code, Cursor, and Gemini CLI), autonomous Go-To-Market (GTM) research agents, and developer monetization platforms exists specifically to guide technical founders through software commercialization, positioning, pricing, and B2B sales execution. 

3.1 Installable Agent Skill Libraries (**SKILL.md**) for Commercialization, Positioning & Pricing 

These open-source repositories follow the standard SKILL.md format, allowing you to drop them into .claude/skills/, .cursor/skills/, or your agent skill manager to run structured commercialization workflows directly against the OdbDesign repository:

OdbDesign Commercialization & License Grant Readiness Guide Page 8 of 12   
Skill Pack / Repository Commercialization Capabilities   
Pros & Cons for OdbDesign   
How to Run It on OdbDesign 

LeadMagic GTM Skills 

(**LeadMagic/gtm-skills**) 

PM Skills Marketplace (**phuryn/pm skills**) 

Obviously Awesome Positioning Skill (**wondelai/skills**) 

Product Manager Skills 

(**deanpeters/Product-Manager Skills**)   
211 self-contained GTM skills across 25 categories, 

including 

engineer-to 

founder, 

developer-gtm, solo-founder gtm, pricing   
strategy, icp scoring, and 

battlecard 

builder. 

69 product 

management skills and 42 chained workflows across 9 plugins (pm 

product 

strategy, pm-go to-market, pm market 

research), 

including 

/pricing, /plan launch, 

/business-model, and 

monetization strategy. 

Implements April Dunford's 5-step B2B positioning methodology and 10-part Positioning Canvas, translating low-level technical features into 

executive value themes. 

77 framework 

driven skills 

including a 14-skill autonomous Market Intelligence Suite (market   
landscape-scan, competitive 

research 

snapshot, 

pricing   
Pros: Purpose built for solo 

technical 

founders 

converting an engineering repo into a B2B 

product. 

Cons: Very large repo (1,446 files); install specific sub-skills rather than loading all at once. 

Pros: MIT 

licensed; native Claude Code and Cursor plugin 

manifests; strong B2B monetization and beachhead segment 

frameworks. 

Cons: Chained slash commands run natively in Claude Code; 

Cursor invokes individual   
SKILL.md files. 

Pros: Evaluates "build an internal ODB++ parser in house" as the primary 

competitive 

alternative—the exact decision Keiron is making right now.   
Cons: Focused on positioning narrative rather than   
contract/licensing math. 

Pros: Uses an "Adaptive   
Decision Ladder" that asks 3–5 targeted context questions before generating 

outputs and tags every claim as Fact, Inference, or Assumption.   
Run gtm-context-bootstrap on the OdbDesign repo to generate a persistent ICP/positioning file, then 

invoke engineer-to-founder and developer-gtm. 

Run /business-model and /pricing to stress-test your OEM machine-bundling vs. Enterprise Server \+ Seat pricing tiers before quoting Keiron. 

Run against docs/one\_pager.md to turn C++17 multi-threaded parsing and Phase 2 Connectivity into an executive ROI brief for Keiron's leadership (Frank Ophelders & Martin de Liefde). 

Run pricing-packaging-tracker and battle-card-builder to benchmark OdbDesign against commercial EDA/CAM SDKs (Valor, DownStream, Ucamco, Cadence).

OdbDesign Commercialization & License Grant Readiness Guide Page 9 of 12   
Skill Pack / Repository Commercialization Capabilities 

packaging 

tracker, tam-sam 

som-calculator).   
Pros & Cons for OdbDesign 

Cons: CC BY NC-SA 4.0 

license (free for internal company use).   
How to Run It on OdbDesign 

Marketing Skills for AI Agents (**coreyhaines31/marketingskills**) 

OneWave Pricing Strategy Designer (**OneWave-AI/claude-skills**) & GTM Technical Product Pricing   
50+ GTM and 

conversion skills (sales   
enablement, 

pricing, 

competitors, 

cold-email, 

launch) anchored by a shared 

.agents/product marketing.md context file. 

Evaluates 5 pricing models (value   
based, cost-plus, competitor, tier expansion triggers) and developer-tool packaging 

boundaries (open source core vs. paid commercial   
entitlements).   
Pros: Strong 

sales 

enablement skill for generating B2B one-pagers, objection 

handling guides, and outbound sequences. 

Cons: Some skills target web CRO rather than   
compiled C++ SDKs. 

Pros: Forces 

clear value 

metric selection (per-machine, per-seat, server node) and   
models annual cash-flow impact. Cons: Requires feeding in your SDK maintenance and support cost floor.   
Install via npx skills add coreyhaines31/marketingskills and invoke sales-enablement to generate your commercial licensing deck and FAQ sheet. 

Use alongside competitor-price tracker to finalize your 3-tier OEM and Enterprise rate card. 

3.2 Autonomous B2B GTM Agents, Market Research Apps & Monetization Platforms 

Beyond local coding-agent skills, these specialized SaaS agents and open-source commercialization platforms handle prospect discovery, competitive intelligence, and automated license/tax fulfillment: 

Platform / Agent Category & Core Function   
Pros & Cons for OdbDesign Actionable Use Case for OdbDesign 

Clay \+ Claygent (**clay.com**) 

Perplexity Deep Research 

(**perplexity.ai**)   
Autonomous B2B lead enrichment & account research agent chaining 75+ data sources with web-crawling AI 

(Claygent). 

Multi-step autonomous web and filing research agent with dedicated Spaces and 10-K   
competitive 

benchmarking.   
Pros: Can automatically crawl SMT/PCB equipment makers (like Keiron, Mycronic, Koh Young, ASMPT) to verify if their job postings or docs mention ODB++, IPC-2581, or C++. 

Cons: Credit-based cost; only needed when shifting from inbound to outbound sales. 

Pros: Low cost (\$20/mo); rapidly investigates inbound corporate leads (funding rounds, leadership, export control flags, pricing 

benchmarks) with citations.   
Use Claygent to build a list of 50 other SMT/AOI/SPI hardware OEMs worldwide who need an embedded ODB++ job 

preparation parser just like Keiron. 

Run Deep Research on every new inbound licensing lead to verify corporate funding, employee headcount, and parent-company structure before quoting.

OdbDesign Commercialization & License Grant Readiness Guide Page 10 of 12   
Platform / Agent Category & Core Function   
Pros & Cons for OdbDesign Actionable Use Case for OdbDesign 

Cons: Requires manual review 

of legal/contract outputs. 

Copy.ai GTM AI 

Platform (**copy.ai**) 

Polar (**polar.sh**) & Paddle (**paddle.com**) 

Open Core Ventures Handbook & Heather Meeker's Dual 

Licensing Guides   
Multi-LLM B2B GTM workflow platform for turning technical   
documentation into sales playbooks, account briefs, and outreach sequences. 

Developer Merchant of Record (MoR) platforms handling global VAT/sales tax compliance,   
subscription invoicing, private GitHub repo invites, and binary/key delivery. 

Industry-standard open core / dual-licensing playbooks defining 

feature tiering by buyer persona (IC vs. Manager vs. Executive/OEM).   
Pros: Free tier / \$29/mo Chat plan; fast generation of B2B sales collateral from technical specs. 

Cons: Enterprise workflow tiers are priced for full sales teams. 

Pros: Polar is built specifically for open-source maintainers monetizing repos—automates VAT, invoices, and private repo/artifact access with zero monthly base cost. 

Cons: Takes a small 

transaction percentage; large enterprise PO wires can still go direct to your LLC bank account. 

Pros: Free, battle-tested strategic frameworks from GitLab co-founder Sid 

Sijbrandij (OCV Operating Principles) and OSS attorney Heather Meeker. 

Cons: Conceptual playbooks rather than automated software.   
Feed docs/one\_pager.md and this readiness guide into Copy.ai to generate   
standardized follow-up 

sequences and executive summaries. 

Set up a Polar.sh organization for OdbDesign to host self serve commercial 

evaluation/SDK subscriptions and automated binary/SBOM downloads, plus use Paddle's Price Intelligently B2B Pricing Guide. 

Apply Buyer-Based Open Core to keep single-user AGPL-3.0 parsing open while placing OEM redistribution, multi-user gRPC/REST server   
management, and IPC-2581 enterprise features in the commercial tier. 

Recommended Execution Sequence: 5 Steps to Declare Immediate Readiness for Keiron 

You do not need to build every long-term automation tool before replying to Ruben van Oers at Keiron Printing Technologies (Keiron Email Thread). Follow this two-stage sequence to reply to Keiron immediately while completing the underlying library and legal preparation in parallel: 

1\. Step 1 — Lock In Your 4 Core Decisions (Today, 15 Minutes):

> **Status (2026-11-07): Partially done.** Technical decisions are locked (Ed25519 offline licensing; watermark-not-keyfile for OEM DLLs; loopback-locked evaluation; **private delivery only** — SDK archives are never public release assets). Entity, licensing model, prices, and support-tier pricing remain open user decisions (items 1.1, 2.2, 2.3).

Licensor Entity: Confirm using The Intel Agency (or forming OdbDesign LLC) as the contracting entity. 

Keiron Licensing Model: Present two clean options to Keiron: (a) Annual OEM Machine-Bundling Subscription (\$15k–\$28k/yr tiered by annual HF2 machine volume, or \$45k/yr unlimited, including all updates and perpetual end-customer machine runtime rights) and (b) Base R\&D SDK Subscription (\$7.5k/yr) \+ Per-Machine Royalty Packs (\$250–\$300/machine). 

Support Tiers: Offer Standard Maintenance (included), Silver Production Support (+\$6k/yr, 2-business day P1 response, 15 hrs/yr cap), Gold OEM SLA (+\$18k/yr, 1-business-day P1 response, hotfix backports, 40 hrs/yr cap), and Custom Engineering SOWs (\$250/hr). 

Onboarding Process: Define a 4-step process: (1) Technical Scoping Call / Intake Questionnaire → (2) Mutual NDA & 30-Day Evaluation License (with public-domain sample\_design and time-locked evaluation build) → (3) Commercial Order Schedule Selection → (4) Production Watermarked SDK/Binary & SBOM Delivery.

OdbDesign Commercialization & License Grant Readiness Guide Page 11 of 12   
2\. Step 2 — Send the Initial Reply & Commercial Overview One-Pager to Keiron (Day 1):

> **Status (2026-11-07): Not started — deliberately out of the approved engineering scope.** The one-pager source material now exists (`COMMERCIAL-LICENSE.md`); the reply itself is the user's business action. Reply to Ruben van Oers (CC'ing Frank Ophelders and Martin de Liefde) answering his three questions directly, attaching a 2- page OdbDesign Commercial OEM & Enterprise Licensing Overview PDF, and inviting their team to book a 30- minute technical & licensing discovery call via your Google Calendar appointment link. 

3\. Step 3 — Prep the **OdbDesign** Repository for Clean SDK & Compliance Delivery (Days 2–3):

> **Status (2026-11-07): Complete — PR #610** (`nam/commercial-readiness` → `nam20485`, 17 files, every check green incl. the full Windows/Linux/macOS CMake matrix and test-results jobs; awaiting merge). Added: install rules + generated-header packaging, `OdbDesignParser.h` parser-only entry, `OdbDesignServerLib` split (overlay prerequisite), gRPC bind fix, `THIRD_PARTY_NOTICES.txt` + generator, `COMMERCIAL-LICENSE.md`, CLA text, SBOM artifact upload, OneDrive-path cleanup. `sample_design.tgz` was already public in `OdbDesignTestData` (see errata). Add CMake install() rules in OdbDesignLib/CMakeLists.txt to output a clean OdbDesign-SDK Windows-x64.zip / Linux-x64.zip package containing headers, generated Protobuf headers, OdbDesign.dll/.lib/.so, and runtime vcpkg DLLs. 

Generate and commit THIRD\_PARTY\_NOTICES.txt (covering libarchive, zlib, Crow, Protobuf, and gRPC) and COMMERCIAL-LICENSE.md, and enable CLA Assistant on GitHub. 

4\. Step 4 — Implement the Private Commercial Overlay & OpenSSL **Ed25519** Verifier (Days 4–5):

> **Status (2026-11-07): Complete — private repo `nam20485/OdbDesign-Commercial`.** LicenseGuard (Ed25519/EVP, 13/13 tests), license-gated `OdbDesignCommercialServer` (verify-before-bind; loopback override proven against `--bind 0.0.0.0`), watermark + strip, `tools/make-license.sh` signer (dev keypair generated; production key rotation documented), packaging workflow, runbook. End-to-end gauntlet green: valid → runs on 127.0.0.1 (`/healthz` 200); expired/tampered/no-license → exit 1 with nothing bound; watermark visible; symbols stripped. Create the private commercial build branch that strips symbols, embeds the customer watermark UUID, and checks a signed license.json certificate via OpenSSL EVP\_DigestVerifyInit for 30-day evaluation builds and OdbDesignServer deployments. 

5\. Step 5 — Finalize the Modular Commercial License Grant Agreement (Days 5–6):

> **Status (2026-11-07): Not started — legal drafting deferred** (items 4.3/4.4). Prerequisite for any binding grant; nothing technical blocks it. Prepare the standard Mutual NDA / Evaluation Agreement and the Master Commercial Software License Agreement (with Schedule A for Hardware OEM Bundling and Schedule B for Enterprise Server \+ Seats, including the Siemens ODB++ Solutions Alliance disclaimer) ready for signature. 

## Execution Status Update — November 7, 2026

On 2026-11-07 the engineering workstreams of this guide (execution-sequence Steps 3–4, per the approved `commercial-readiness-execution-plan.md`) were implemented and verified end-to-end. User scope decisions taken at plan approval: **repo readiness + private overlay** (business collateral excluded), and **SDK distribution = private delivery only** (SDK archives are never public release assets; per-customer after NDA/evaluation agreement). Public-repo changes ride **PR #610** (`nam/commercial-readiness` → `nam20485`; 17 files; all checks green including the full Windows/Linux/macOS CMake matrix); enforcement lives in the **private** repository `nam20485/OdbDesign-Commercial`.

### Master status table

| Item | Domain | Status |
|---|---|---|
| 1.1 Licensor entity | Legal/IP | **Deferred (user)** |
| 1.2 Employer CIIAA separation | Legal/IP | **Deferred (ongoing discipline)** |
| 1.3 Chain-of-title / CLA | Legal/IP | **In progress** — CLA text done (PR #610); CLA Assistant enablement pending |
| 1.4 Dual-licensing framework | Legal/IP | **Complete** (AGPL-3.0 kept + `COMMERCIAL-LICENSE.md`) |
| 1.5 Third-party notices | Legal/IP | **Complete** (`THIRD_PARTY_NOTICES.txt` + generator) |
| 1.6 ODB++ format-IP disclaimer | Legal/IP | **Complete in collateral**; contract clause rides 4.4 |
| 1.7 Export control / KYC | Legal/IP | **Partial** (compliance notes; screening workflow open) |
| 2.1 SKU boundaries | Product | **Technical complete; SKUs/pricing deferred (user)** |
| 2.2 Licensing model | Product | **Deferred (user)** — enforcement is model-agnostic |
| 2.3 Pricing tiers | Product | **Deferred (user)** — nothing published by design |
| 2.4 Evaluation policy | Product | **Technical complete** (30-day + loopback enforced); paper = 4.3 |
| 2.5 Escrow | Product | **Deferred** (binary-only default in effect) |
| 3.1 Private build overlay | Technical | **Complete** — private repo + superbuild + packaging workflow |
| 3.2 License enforcement | Technical | **Complete** — LicenseGuard Ed25519 (Option 2), 13/13 tests |
| 3.3 Watermark + strip | Technical | **Complete** (Option 1) — proven live |
| 3.4 SDK packaging | Technical | **Complete (install basis)**; `find_package` config + workflow dry-run open |
| 3.5 SBOM bundling | Technical | **In progress** — SPDX artifact upload landed; per-delivery bundling open |
| 3.6 Fixture governance | Technical | **Complete — verified sound** (no leak found; sample already public) |
| 4.1 Licensing one-pager | Collateral | **In progress** — markdown done; customer PDF deferred |
| 4.2 KYC questionnaire | Collateral | **Deferred** |
| 4.3 NDA / Eval / China NNN | Legal | **Deferred** |
| 4.4 MCLA + schedules | Legal | **Deferred** |
| 5.1 Billing / tax stack | Ops | **Deferred** |
| 5.2 Fulfillment / SLA tiers | Ops | **Partial** — mechanism + structure built; contracts open |

### Decisions made during execution (beyond the guide's recommendations)

1. **The public repository contains zero license-enforcement code.** An AGPL user could strip any check anyway; dual-licensing rests on 100% copyright ownership (CODEOWNERS re-verified) + the CLA. Enforcement is a convenience layer for evaluation/server builds and lives entirely in the private overlay.
2. **Separate private repo, not a "commercial branch."** Every ref of a public repository is fetchable by anyone who clones — a private branch is not private. `nam20485/OdbDesign-Commercial` superbuilds upstream via a git submodule instead.
3. **License-gated server instead of a patched upstream `main`.** The overlay owns a ~110-line `CommercialServerMain.cpp` that links the (new) upstream `OdbDesignServerLib`, verifies the license **before constructing the server app**, then delegates — no upstream source patching to maintain.
4. **Loopback enforcement required a public-repo fix.** The upstream gRPC listener hardcoded `0.0.0.0`, ignoring `--bind` (REST honored it) — a locked build could be exposed via the gRPC port. Fixed in PR #610; default behavior unchanged.
5. **Price-free `COMMERCIAL-LICENSE.md`.** Pricing is a live user decision; the public overview documents process/grants/restrictions only.
6. **Watermark for OEM production DLLs (guide Option 1); license file for eval + server (Option 2); Option 3 deferred** — same Ed25519 scheme, so Keygen.sh/LicenseSpring adoption later needs no rework.
7. **Notices carry license text, not versions** — exact versions live in `vcpkg.json` pins + the per-build SPDX SBOM (regenerated per delivery), keeping the committed file stable.
8. **Dev signing keypair committed as the embedded verification key** (public half only; private half gitignored at `keys/dev-signing.key`). Production rotation procedure documented in the overlay README; rotation invalidates all previously issued licenses by design.

### Implementation record & verification evidence

- **Public repo (PR #610):** `OdbDesignServerLib` STATIC split + thin `main.cpp`; gRPC bind fix; first-ever `install()` rules (targets/headers/26 generated `.pb.h`; `INSTALL_GTEST OFF` after googletest's install rules leaked into the SDK tree); `OdbDesignParser.h` + PCH-exempt `ParserOnlyHeaderTests` (TU includes only the parser header); notices + generator; `COMMERCIAL-LICENSE.md`; `.github/CLA.md` (ICLA + CCLA, relicensing grant §1.2(2)); `docs/CONTRIBUTING.md`, `docs/README.md` ×2, `docs/one_pager.md` links; SBOM artifact upload; `OdbDesignApp` OneDrive-path → `ODB_TEST_DATA_DIR`.
  *Verified:* full `ctest --preset linux-dynamic-release` 0 failures; `cmake --install` layout check; 20-line out-of-tree consumer (parse + ProductModel: 644 nets / 71 packages) with no Crow flags; live server on loopback (REST `127.0.0.1:8888`-style + gRPC `[::ffff:127.0.0.1]`); CI matrix green on all three platforms.
- **Private overlay (`nam20485/OdbDesign-Commercial`):** superbuild + submodule; `LicenseGuard` (canonical-JSON payload, Ed25519 over exact payload bytes, base64 signature wrapper; expiry/fields/product checks; `VerifyStringWithKey` test-injection seam; 13/13 tests incl. committed dev-key fixture); `OdbDesignCommercialServer` (license path via `--license-file` / `ODBDESIGN_LICENSE_FILE`; `loopback_only` rewrites argv `--bind` → `127.0.0.1` and the fixed upstream gRPC listener follows); CMake watermark generator + `ODBDESIGN_WATERMARK_SDK_DLL` injection into the shared library; `tools/make-license.sh` (jq `-c -S` canonicalization; openssl `pkeyutl -sign -rawin` **requires `-in <file>`** — stdin fails with "unable to determine file size"); manual-dispatch packaging workflow (GHA vcpkg binary cache; GPG-signed archives; 14-day private artifacts); delivery runbook with per-delivery verification checklist.
  *Verified (live gauntlet):* valid license → REST+gRPC bound to 127.0.0.1, `/healthz` 200 — **including when `--bind 0.0.0.0` was explicitly passed**; expired / tampered / no-license → exit 1 before any socket; `strings` shows `ODBDESIGN-COMMERCIAL:…` watermark; `nm`/`file` confirm stripped; superbuild re-verified against the pinned submodule.

### Deferred items — key points and next steps

**Legal / corporate (user + counsel):**
- 1.1 entity: confirm The Intel Agency (or form WA LLC) + 1-page internal IP license — *before any binding quote*.
- 4.3 / 4.4 agreements: Mutual NDA + 30-day Eval Agreement, bilingual China NNN, MCLA + Schedule A (OEM bundling) + Schedule B (Enterprise Server + Seats), carrying the Siemens ODB++ disclaimer and EAR clause. Nothing technical blocks these.
- 1.3 remainder: enable CLA Assistant in the browser (signatures stored in a side repo, pointing at `.github/CLA.md`) **before accepting any external PR**.

**Commercial decisions (user):**
- 2.2 / 2.3 / 5.1: pick licensing model(s) and price points from §2 (Model A/B for Keiron; Model C for enterprise/server); decide PO+wire vs. Polar.sh. The enforcement side already supports any combination via license-payload fields.

**Technical follow-ups (small, sequenced):**
- Merge PR #610 (merge commit only), then bump the overlay submodule to the merge commit.
- Add `GPG_PRIVATE_KEY` + `GPG_PRIVATE_KEY_PASSPHRASE` secrets to the private repo, then run one packaging-workflow dry run to prove the per-customer archive end-to-end.
- Bundle the SPDX SBOM into each delivery (pull the new workflow artifact or run `syft` in the packaging workflow) — closes 3.5.
- `OdbDesignConfig.cmake` (+ `installPresets`) for `find_package(OdbDesign)` consumption — closes the 3.4 follow-up.
- Produce the Keiron one-pager PDF/deck from `COMMERCIAL-LICENSE.md` once pricing is set — closes 4.1.

### Errata — corrections to this guide discovered during implementation

1. §3 claimed "`.github/workflows/create-release.yml` already builds multi-platform archives" — it **repackages** artifacts from `cmake-multi-platform.yml` and did **not** build anything itself; more importantly the released archives were binaries-only (no headers, no Windows `.lib` import libraries) — nothing a customer could compile against. The install rules + packager close that gap.
2. §3's "private commercial repository/branch" — a *branch* of a public repo is not private (all refs are fetchable). A separate private repository is required; implemented that way.
3. §3.6 implied `sample_design` needed isolating/publishing — the audit found it **already public** in `OdbDesignTestData` (and proprietary designs never published). The GitHub tree-API listing that suggested otherwise was truncated; verify with `git ls-files` locally.
4. Undocumented prerequisite discovered: the server's app glue (controllers/services/app base) compiled only into the executable target — the `OdbDesignServerLib` split (PR #610) was required before any overlay could link the server without patching sources.
5. SDK consumers must also link `protobuf` (public headers instantiate protobuf templates inline) — documented in the PR and consumed by the packager's link line.

### Artifacts

| Artifact | Where |
|---|---|
| Public-repo changes | PR **#610** → `nam20485` (branch `nam/commercial-readiness`, commit `6210879`) |
| Docs (this guide + execution plan + program status) | `nam20485` @ `6b3cf86` |
| Private overlay | `nam20485/OdbDesign-Commercial` (private; submodule @ `6210879`) |
| Engineering plan | `docs/plan/commercialization/commercial-readiness-execution-plan.md` |

---

References & Verified Sources 

Workspace Sources: Keiron Printing Technologies OdbDesign Inquiry (Nov 5, 2026\) · LinBoard3D Studio Commercial License Inquiry (Oct 27, 2026\) · OdbDesign Inquiry Entity Investigation (Google Doc) · LinBoard3D Studio’s OdbDesign Licensing Inquiry Stance (Google Doc) · Cincinnatus LLC CIIAA Attachment A · Nathan Miller Master Resume. 

Counterparty & Legal/IP References: Keiron HF2 LiFT Printer Product Page · Keiron €20.7M Series A Release · GNU AGPL v3.0 License · Washington RCW 49.44.140 (Employee Inventions) · Washington LLC Act Liability Shield (RCW 25.15.126) · Siemens ODB++ Solutions Development Partnership Terms of Use · Apache Contributor License Agreements · CLA Assistant for GitHub · U.S. BIS Interactive Commerce Control List · China NNN Agreements Guide · Codekeeper Software Escrow Analysis · Escode (formerly NCC Group) Software Escrow. 

Technical & Licensing Platforms: OpenSSL EVP\_SIGNATURE-ED25519 · OpenSSL EVP\_DigestVerifyInit · Keygen C++ Ed25519 Offline License Verification · Keygen.sh Offline Licensing · LicenseSpring C++ SDK · Polar.sh Merchant of Record · Paddle B2B SaaS Growth & Pricing · Open Core Ventures Pricing Model · Functional Source License (FSL) · PolyForm Noncommercial License. 

AI Commercialization Skills & Agents: LeadMagic GTM Skills · PM Skills Marketplace · Obviously Awesome Positioning Skill · Dean Peters Product Manager Skills · Marketing Skills for AI Agents · OneWave Pricing Strategy Designer · GTM Technical Product Pricing · GTM Agents Marketplace · Clay & Claygent · Perplexity Deep Research · Copy.ai GTM AI Platform.

OdbDesign Commercialization & License Grant Readiness Guide Page 12 of 12 