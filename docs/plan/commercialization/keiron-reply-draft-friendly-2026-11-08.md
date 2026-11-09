# Keiron reply — DRAFT, friendly version (2026-11-08)

*One of two drafts — the shorter, more formal alternative is `keiron-reply-draft-formal-2026-11-08.md`.*

> **Send by 23:00 PST Sunday 2026-11-08** (= 08:00 CET Monday 2026-11-09) so it is first in their inbox Monday morning.
> **To:** Ruben van Oers <r.vanoers@keirontechnologies.com>
> **Cc:** Frank Ophelders, Martin de Liefde
> **Subject:** Re: ODB++ library — commercial licensing (OdbDesign)
> **Attachments:** none (deliberate — see notes below)

---

Hi Ruben,

Thank you for reaching out, and my apologies for the brief delay — I wanted to give your questions a proper answer rather than a rushed one. I'm genuinely glad to hear OdbDesign caught your interest for the HF2 LiFT printer's job preparation software. Having OdbDesign integrated into a next-generation SMT platform like the HF2 is exactly the kind of application it was built for, and I'd be very happy to work with Keiron on making that happen.

Let me answer your three questions directly:

**1. Process for obtaining a closed-source / commercial license**

The typical path is:

- A **scoping call** (30–45 min) so I understand your integration, platforms, and support expectations, and you can evaluate the fit directly with me — I'm the author and maintainer, so there's no layers-between-you-and-engineering.
- If useful, a **mutual NDA**, after which I can issue a **fully functional evaluation license** — typically 30 days, watermarked binaries, so your team can validate the integration on real job data under a clear entitlement — followed by a tailored **commercial proposal** (license terms, pricing, support options).
- **Agreement and delivery**: licensed binaries (SDK / server, SBOM and third-party notices included), with updates during the subscription term. Agreements are executed through Artificial Intelligence Agency, my company.

So the engineering evaluation and the commercial track can run in parallel: once the NDA is in place, the evaluation build can be in your hands the same day.

**2. Available license models**

I keep this flexible and will shape it to how Keiron actually builds and ships rather than force a one-size package. The usual structures for an equipment OEM are:

- an **annual OEM subscription** tiered by machine volume (all updates and support included), or
- a **base SDK subscription + prepaid per-machine runtime packs** — attractive if first-year HF2 volumes are still ramping.

Larger deployments of the REST/gRPC server (e.g. a central job-prep or line-engineering server) can be licensed per server instance + seats if that matches your architecture. Delivery is **binary-only** (the public source remains available under AGPL, and your closed-source integration is exactly what the commercial license enables).

**3. Future support options**

Support is a first-class part of the offer, not an afterthought, and I'm open to shaping it around your production requirements:

- **Standard maintenance** is included with any active subscription: binary updates, security/CVE patches, SBOM refreshes, and e-mail support.
- **Premium support tiers** with committed response times (e.g. next-business-day or same-day for production-down issues) and a defined annual engineering-hours budget — useful once HF2 systems are in the field.
- **Custom engineering / SOWs** if you need specific extensions (e.g. data or workflow features particular to LiFT job preparation) — as the author, I can turn those around quickly.
- Compliance artifacts your organization may require — SBOMs (SPDX), signed releases, export documentation — can all be provided; tell me what your processes expect and we'll accommodate it.

**To tailor the right package, a few questions** (happy to take these on a call instead):

1. **Integration shape** — C++ library linked directly into the job-prep application, a local REST/gRPC sidecar process, or a central server serving multiple stations?
2. **Platforms** — Windows and/or Linux (versions/toolchain), CPU architecture, any real-time constraints?
3. **API surface** — C++ SDK, gRPC, or REST; protobuf-based data access; any must-have endpoints or data you already know you'll need?
4. **Volume & timeline** — expected HF2 shipment volumes (first year and steady state), and when the release that bundles OdbDesign is planned?
5. **Support expectations** — what response times and update cadence matter to you? Timezone coverage (I'm US Pacific, but early-morning CET calls work fine)?
6. **Commercial & compliance** — procurement process (purchase orders?), the Keiron entity that would sign, and any contractual requirements (liability caps, warranties, compliance documents) I should prepare for?

If it's easiest, grab any slot that suits you here: [scheduling link — see docs/README.md Google Calendar button]. EU mornings generally work well for me.

Thanks again for the interest, Ruben — congrats again on the HF2 and the Series A. I'm looking forward to working with you and the team.

Best regards,

**Nathan Miller**
Author & Maintainer, OdbDesign · Artificial Intelligence Agency
nmiller@intel-agency.com · [calendar link] · https://github.com/nam20485/OdbDesign

---

## Notes for the sender (not part of the email)

- **Signature/from-address (S1, decided 2026-11-08):** reply from **nmiller@intel-agency.com**, sign "Author & Maintainer, OdbDesign · Artificial Intelligence Agency". Registered entity name is **Artificial Intelligence Agency LLC** — email/marketing may drop the "LLC" (as here); contracts, POs, and invoices always use the full registered name. "The Intel Agency" is an informal nickname from the domain, not for commercial documents. Domain verified for deliverability (Google Workspace MX, SPF, DMARC).
- **No pricing** in this email by design (checklist S3): numbers go in the tailored proposal after the scoping call, once B1/B2 are decided.
- **No attachment** (S0): a commercial overview PDF (C6) is worth attaching only after pricing is set; the email is self-contained and the repo link carries the technical credibility.
- Replace the `[scheduling link]` (body) and `[calendar link]` (signature) placeholders with your live Google Calendar URL from `docs/README.md`.
- Evaluation framing (guide 2.4, amended 2026-11-08): licensed 30-day eval under NDA — explicit entitlement, watermarked builds. If a prospect independently raises building from the public AGPL source for internal evaluation, the position is: that is legally permitted for internal evaluation only, and any distribution or production use requires the commercial license. We do not advertise it.
- If you'd rather not expose all six scoping questions in writing, trim to 1–4 and take 5–6 on the call.
