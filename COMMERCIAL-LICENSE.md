# OdbDesign Commercial Licensing Overview

OdbDesign is available under a **dual-license** model:

- **Open source (default):** the [GNU Affero General Public License v3.0](LICENSE) (AGPL-3.0). Free to use, study, modify, and redistribute, provided you comply with AGPL-3.0 terms — including network-use source-disclosure obligations and copyleft redistribution requirements.
- **Commercial license (optional):** a fee-bearing license that permits use and redistribution of OdbDesign inside **closed-source** products — OEM machine software, desktop applications, and server deployments — without any AGPL-3.0 obligations.

If your use case is closed-source or embedded, you need the commercial license. This document describes what it covers and how to obtain it.

## What the commercial license grants

Depending on the ordered schedule, a commercial license may include:

- **OEM / SDK schedule** — the native C++17 shared library (`OdbDesign.dll` / `libOdbDesign.so`) with public headers, to integrate into your own closed-source application (e.g. job-preparation or DFM software shipped with a product or machine).
- **Server schedule** — `OdbDesignServer` (REST + gRPC) for deployment as an on-premise enterprise server or as a local loopback sidecar inside your product.
- Binary distribution including required third-party runtime libraries, a Software Bill of Materials (SBOM), and third-party notices.
- Updates and security patches released during an active subscription term.
- Optional support and maintenance tiers, and custom-engineering statements of work.

## What the commercial license does NOT grant

- **No ODB++ format or trademark rights.** OdbDesign reads and writes the ODB++ format, but the ODB++ format, specification, and trademarks belong to their respective owners (Siemens EDA). Licensees who need to use ODB++ trademarks or participate in the ODB++ ecosystem should register directly with the ODB++ Solutions Alliance. Nothing in an OdbDesign license transfers any third-party format IP.
- **No source-code rights unless expressly ordered.** Commercial licenses are binary-only by default. Source-code escrow through a third-party escrow agent is available as an ordered option (escrow fees paid by the licensee).
- **No exclusivity.** OdbDesign is licensed non-exclusively; the licensor remains free to license it to others and to continue the open-source distribution.

## Licensing process

1. **Technical scoping** — a short call or questionnaire covering your deployment model (embedded SDK vs. server), platforms, and expected volume, so the right schedule and terms can be quoted.
2. **NDA + evaluation (optional)** — mutual NDA, then a time-limited evaluation build (typically 30 days) with technical evaluation restrictions (e.g. loopback-only binding, watermarking) plus a public-domain sample ODB++ archive for validation.
3. **Order schedule** — select the license schedule (OEM bundling, enterprise server, etc.), support tier, and term; commercial terms are confirmed in a signed agreement.
4. **Delivery** — licensed binaries, headers, SBOM, notices, and license certificate are delivered for your build/integration; invoicing per the agreed payment terms.

## Support & maintenance

Active subscriptions include binary updates, CVE patches, and SBOM refreshes. Paid support tiers with defined initial-response times (business hours) and annual engineering-hour caps, plus custom-engineering statements of work, are available — see your quote for current tiers and pricing.

## Compliance notes

- **Export control:** OdbDesign is believed to be EAR99-classified mass-market software. Exports are subject to applicable export-control laws; the licensor screens counterparties against applicable denied-party lists before granting licenses.
- **Third-party components:** OdbDesign links permissively-licensed third-party libraries (e.g. libarchive, zlib, Crow, Protobuf, gRPC — see [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt)). Their notices ship with every binary distribution.

## Contact

To discuss a commercial license, evaluation, or support: open an issue against the repository, or use the scheduling link in the [documentation](docs/README.md).

---

*This overview is informative and not an offer or a binding license. Commercial licenses are granted exclusively by executed agreement; if any conflict arises between this document and an executed agreement, the agreement governs.*
