# Reference

Durable reference material — specs, ops runbooks, research, and vendor docs.
Not plans: see [`docs/plan/`](../plan/README.md) for active work.

* **`architecture.md`** — OdbDesignServer architecture overview (current architecture + dual REST/gRPC API design).
* **`tech-stack.md`** — technology stack catalog (current core technologies + planned additions).
* **`monitoring-grafana-prometheus-trivy.md`** — ops runbook for the deployed Grafana/Prometheus/Trivy stack (`deploy/helm/values-prom.yaml`, `values-trivy.yaml`, `scripts/deploy-monitoring.ps1`).
* **`client-debugger-triage.md`** — how-to for diagnosing symbol/units issues via the live diagnostics endpoint.
* **`odbpp/`** — ODB++ format material: the spec PDF (`odb_spec_user.pdf`), pointer doc (`odb-spec.md`), attribute spec excerpt (`ODB++ Attributes.txt`), attribute-index spec citations (`odb-spec-attribute-lookup-confirmation.md`), and `images/` (implementation-state diagram used by [README.md](../README.md), project infographic).
* **`research/`** — ODB++ shape/feature research and REST API consumer docs: `Features Shapes.md` (extended master), `API.md` + `ODB_REST_API_INTEGRATION.md` (API contract for consumers), `ODB_SHAPE_DEFINITIONS.md`, `ODB++_Shape_Representation_Research.md`.
* **`vcpkg/`** — vcpkg binary caching: `localdev-gh-binary-cacheing.md` is the **living developer guide** (matches `scripts/setup-vcpkg-cache.ps1/.sh`); the rest are Microsoft Learn tutorial copies and the `vcpkg-configuration.json` format reference.
