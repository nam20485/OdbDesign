---
title: Licensing & IP Protection
description: How OdbDesign's dual-license model works, and how the codebase is kept safe for closed-source commercial licensing
---

# Licensing & IP Protection

*How the OdbDesign codebase is made safe for closed-source commercial licensing — and why the commercial license is fully self-contained, separate from the FOSS codebase, and verifiable from public evidence.*

**100%** owner-authored commercial tree · **281/281** tests green after restoration · **16** permissively-licensed dependencies · **0** copyleft dependencies anywhere

## The thesis

OdbDesign ships under a **dual-license model**: the public repository remains free and open source under **AGPL-3.0**, forever — and a **fully self-contained commercial license**, granted solely by the copyright owner, authorizes closed-source use. Every action described below was taken so that the commercial grant derives from **one clean chain of title** with no dependence on anyone else's open-source contributions. The result: a prospective licensee's legal team can verify safety from public evidence alone.

## Background: what dual licensing is

Most software carries one license. **Dual-licensed software carries two**, and you choose which one governs your use:

- The **open-source license** (here AGPL-3.0) lets anyone use, study, and modify the code for free — with one core condition, *copyleft*: if you distribute the software, or run it as a network service for others, you must make your integration's source code available under the same terms. For an open project that is a fair trade; for a company whose product is closed-source, it is a non-starter — which is exactly why AGPL sits on many corporate deny-lists.
- The **commercial license** exists for that case: a standard fee-bearing license from the copyright owner that permits closed-source use and redistribution with **no copyleft obligations at all**. You keep your code, your product, and your IP entirely private.

The two never mix in one product — pick a lane per project, same as Qt. And to be clear about what this is: **not a novel legal experiment**. Dual licensing is a decades-old, mainstream structure. If you have used a car infotainment system (Qt), an appliance with an embedded database (MySQL), or enterprise software that produces PDFs (iText), you have already shipped or consumed products built on exactly this model.

## The model

| Aspect | FOSS edition | Commercial edition |
|---|---|---|
| License | AGPL-3.0 (public repository, unchanged forever) | Fee-bearing, closed-source, per customer / OEM |
| Source code | Public repository | Binary-only delivery |
| Use trigger | Free for internal use; copyleft on distribution or network service | Exactly the use AGPL forbids: shipping in a closed product |
| Enforcement | None needed — the license is the grant | Offline license verification + per-customer watermarking (private pipeline) |
| Support | Community | Maintenance, response-time tiers, custom engineering |

The public repository contains **zero license-enforcement code** — an AGPL user could strip any check anyway, so the commercial model rests on copyright and contract, with technical enforcement delivered from a separate private pipeline.

## What was done — the evidence trail

All public, all verifiable:

1. **Dual-licensing framework published.** [COMMERCIAL-LICENSE.md](https://github.com/nam20485/OdbDesign/blob/development/COMMERCIAL-LICENSE.md) defines what the commercial license grants (closed-source OEM/SDK/server use) and what it does not grant (no ODB++ format or trademark rights — those remain with Siemens EDA; no source-code rights). Price-free by design. ([PR #610](https://github.com/nam20485/OdbDesign/pull/610))
2. **Contributor License Agreement introduced — and enforced at the door.** [.github/CLA.md](https://github.com/nam20485/OdbDesign/blob/development/.github/CLA.md) is an Apache-style Individual + Corporate CLA whose grant explicitly includes commercial relicensing rights. CLA Assistant is live: every pull request — the maintainer's own included — is gated by the `license/cla` check; merges are blocked until signed.
3. **Chain-of-title audit.** A full git-history review identified the complete set of non-owner copyright interest: three code changes from two community contributors. Everything else is owner-authored or bot-generated. The ownership surface to clean was small, known, and fully enumerated.
4. **Excision of un-relicensed contributions.** ([PR #623](https://github.com/nam20485/OdbDesign/pull/623)) The two features were removed from the tree with a full public rationale; the affected protobuf wire-field numbers were reserved so they can never silently return under different meaning. The removed code remains in the public AGPL history with credit — no contributor right is touched; it simply no longer feeds commercial builds.
5. **Clean-room restoration of the functionality.** ([PR #624](https://github.com/nam20485/OdbDesign/pull/624)) Both capabilities were reimplemented from scratch, authored exclusively from the ODB++ *Specification* and sample design data — under a written protocol that prohibited reading the removed implementations. The replacements use brand-new wire contracts and are proven by behavioral tests: real-design parses, save→re-parse round-trips, spec-legality negative cases.
6. **Private commercial build pipeline.** A separate private repository superbuilds the public tree and adds what the FOSS edition deliberately lacks: offline license verification, per-customer binary watermarking, and packaging with SBOM and third-party notices. Nothing in the public repo depends on it.
7. **Third-party dependency hygiene.** Every linked library is permissively licensed (BSD, Zlib, Apache-2.0 families). No copyleft dependency exists anywhere in the build; [THIRD_PARTY_NOTICES.txt](https://github.com/nam20485/OdbDesign/blob/development/THIRD_PARTY_NOTICES.txt) is generated mechanically and refreshed per delivery.

## Why the closed-source license is fully self-contained

1. **Sole grantor.** All commercial rights flow from one copyright holder. No committee, no contributor aggregation, no relicensing consent needed from anyone else.
2. **Every line in the licensed tree is one of exactly three things:** owner-authored; covered by a signed CLA; or permissively licensed third-party code with notices attached.
3. **The FOSS edition is unchanged.** Nothing restricts the public AGPL repository — same code visibility, same community rights.
4. **Enforcement travels with the binaries, not the repo:** licensed builds verify offline, are watermarked per customer, and carry their own compliance bundle.
5. **It is all public evidence.** PRs #610, #623, #624, the CLA, and the notices file let any licensee's counsel audit the chain without an NDA.

## A model proven at scale

- **Qt** — the GUI framework shipped in millions of vehicles and industrial HMI systems — is GPL/LGPL plus commercial. Qt's own licensing page sells the commercial edition to OEMs on "retaining your IPR", and expressly prohibits mixing editions in one product: the same closed-source-product⇒commercial-license trigger OdbDesign uses.
- **MySQL** — embedded in countless commercial appliances — runs on the identical dual-license logic; Oracle's own OEM page states that embedding MySQL in a closed-source product requires the commercial license. The model was MySQL AB's engine to a roughly one-billion-dollar acquisition.
- **iText PDF** — the closest license match: **AGPLv3 plus commercial, exactly OdbDesign's pair**. It also mirrors OdbDesign's shape: a single developer's open-source library grown into a global commercial SDK business.
- **Elasticsearch and Redis — moving toward, not away.** In 2024 Elasticsearch *added* AGPLv3 as a licensing option, and Redis 8 (2025) did the same. Two of the most-watched relocations in open-source business history both landed on AGPL as the OSI-accepted leg of a dual offering.

> **For a prospective licensee, in one paragraph.** Using OdbDesign internally under AGPL costs nothing and always will. The moment your product ships with OdbDesign inside — or serves third parties over a network — you need the commercial license, and the grant you receive is clean: one rightsholder, a contributor-tested CLA, a documented excision-and-clean-room record, permissive-only dependencies, and per-customer licensed binaries with SBOM and watermark. The diligence trail is public. Welcome to the same licensing structure that ships Qt in your car.

## Further reading

If you would like more information:

- [The complete story as a printable PDF](assets/OdbDesign-IP-Protection-Story.pdf) — this page in white-paper form, suitable for sharing with legal and procurement teams
- [The GNU Affero General Public License v3.0 — full license text](https://www.gnu.org/licenses/agpl-3.0.en.html)
- [AGPLv3 explained in plain language](https://choosealicense.com/licenses/agpl-v3/) — a friendly, neutral guide to what the license permits and requires

Project evidence: [COMMERCIAL-LICENSE.md](https://github.com/nam20485/OdbDesign/blob/development/COMMERCIAL-LICENSE.md) · [.github/CLA.md](https://github.com/nam20485/OdbDesign/blob/development/.github/CLA.md) · [THIRD_PARTY_NOTICES.txt](https://github.com/nam20485/OdbDesign/blob/development/THIRD_PARTY_NOTICES.txt) · PRs [#610](https://github.com/nam20485/OdbDesign/pull/610), [#623](https://github.com/nam20485/OdbDesign/pull/623), [#624](https://github.com/nam20485/OdbDesign/pull/624)
