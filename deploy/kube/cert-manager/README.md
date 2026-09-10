# OdbDesign ingress/gRPC TLS — cert-manager + private CA (Option C)

Implements the decided strategy from `docs/plan/https-tls-options.md` (Option C,
2026-09-10) and milestone M3.1 of `docs/plan/server-issues-implementation-plan.md`:
cert-manager with a self-managed CA issuing serving certs with **IP SANs**, for the
private k3s VM deployment where no public DNS exists. The same cert Secret serves
both the REST ingress and gRPC TLS termination.

## Components

| File | Purpose |
|---|---|
| `cert-manager.yaml` | Vendored, version-pinned cert-manager static manifest (**v1.17.2**, fetched from the official release URL `https://github.com/cert-manager/cert-manager/releases/download/v1.17.2/cert-manager.yaml`; ~1 MB, 49 resources into the `cert-manager` namespace). Vendored so deploys work offline / from the repo alone. Upgrade by re-fetching a newer pinned release and updating this note. |
| `../issuer-ca.yaml` | Namespace-scoped `Issuer` (kind `CA`, `ca.secretName: odbdesign-root-ca`), namespace `default` — the same namespace every other `deploy/kube` manifest deploys into (they are all default-namespace; the EKS variant pins `default` explicitly). |
| `../certificate-odbs-server.yaml` | `Certificate` → secret `odbdesign-server-tls`; `ipAddresses: [192.168.122.200, 100.118.225.119]`, `dnsNames: [odbdesign.local, odbdesign]`; duration 8760h, renewBefore 720h. |
| `../../scripts/make-tls-ca.sh` | One-time root CA generation (EC prime256v1, SHA-256 only, 10-year root). Refuses to overwrite existing files. |

## One-time setup

### 1. Install cert-manager + issue the cert

Either run `scripts/deploy.ps1 -EnableTls` (installs cert-manager, applies the
issuer/certificate, waits for READY, then continues the normal deploy), or do it by
hand:

```bash
kubectl apply -f deploy/kube/cert-manager/cert-manager.yaml
kubectl -n cert-manager rollout status deploy/cert-manager --timeout=300s
kubectl -n cert-manager rollout status deploy/cert-manager-webhook --timeout=300s

./scripts/make-tls-ca.sh <output-dir>
kubectl create secret tls odbdesign-root-ca --cert=<output-dir>/ca.crt --key=<output-dir>/ca.key -n default

kubectl apply -f deploy/kube/issuer-ca.yaml
kubectl apply -f deploy/kube/certificate-odbs-server.yaml
kubectl wait --for=condition=Ready certificate/odbdesign-server-tls --timeout=300s
```

Verify: `kubectl get certificate odbdesign-server-tls` shows `READY: True`, and
`openssl s_client -connect 192.168.122.200:443` shows a chain that resolves to the
imported CA (after step 2 below).

### 2. Import the CA on clients (once, per client)

`ca.crt` is the only trust anchor to distribute.

- **curl / one-off:** `curl --cacert ca.crt https://192.168.122.200/healthz`
- **Linux (system-wide):** `sudo cp ca.crt /usr/local/share/ca-certificates/odbdesign-ca.crt && sudo update-ca-certificates`
- **Python (requests):** set `REQUESTS_CA_BUNDLE=/path/to/ca.crt` (or rely on the system store after the step above)
- **Windows:** `certutil -addstore -f Root ca.crt` (admin), or import via `certmgr.msc` into *Trusted Root Certification Authorities*
- **grpcurl:** either system trust (after OS import) or per-call `grpcurl -cacert ca.crt ...`

### 3. Keep the root key offline (optional hardening)

The root key currently lives in the cluster as the `odbdesign-root-ca` Secret.
Acceptable for this deployment; for better hygiene the root key can be moved
offline and replaced by an **intermediate CA**: generate an intermediate signed by
the root, import the intermediate (chain: root + intermediate) as the Issuer
secret, and re-issue. Clients then import only the root cert. Not required now;
documented as the upgrade path.

## Deployment flag semantics (scripts/deploy.ps1)

- **No flag (default) = today's behavior, unchanged.** `deploy.ps1` applies
  `deploy/kube/OdbDesignServer/service-grpc-loadbalancer.yaml` (the exact
  LoadBalancer gRPC service that was previously `service-grpc.yaml`) and the ingress.
- **The base `local-ingress.yaml` carries the `tls:` block unconditionally.** This is
  deliberate (simplest correct arrangement): Traefik serves both entrypoints 80 and
  443, so plain HTTP on :80 works with or without the TLS pieces; before the
  `odbdesign-server-tls` secret exists, :443 serves Traefik's default self-signed
  cert — exactly the pre-existing status quo (Option F). `-EnableTls` therefore only
  installs cert-manager + Issuer + Certificate, **waits for the Certificate to go
  READY**, applies the Traefik gRPC entrypoint (HelmChartConfig) + IngressRouteTCP,
  and migrates the gRPC service to ClusterIP.
- **gRPC service type migration:** `-EnableTls` applies `service-grpc.yaml`
  (`ClusterIP`, the IngressRouteTCP backend) instead of the LoadBalancer variant.
  This is effectively a one-way move while the Traefik `grpc` entrypoint publishes
  node :50051 — ServiceLB cannot also bind node :50051 for the LoadBalancer service.
  To revert to the plaintext direct-ServiceLB path: delete the entrypoint config
  (`kubectl -n kube-system delete helmchartconfig traefik` — the HelmChartConfig
  must carry the packaged chart's name; if the cluster had a pre-existing traefik
  HelmChartConfig with other values, restore those instead of deleting) and
  re-run `scripts/deploy.ps1` with no flag (re-applies the LoadBalancer variant;
  k3s re-upgrades Traefik without the extra port when the HelmChartConfig is
  removed).
- **Retired:** the stale `deploy/kube/issuer.yaml` (Let's Encrypt *staging* ACME
  Issuer) was deleted. It never functioned — staging endpoints are deliberately
  untrusted, the ingress host was a bare hostname (ACME-ineligible), and cert-manager
  was never installed (see https-tls-options.md §2). `issuer-ca.yaml` replaces it.

## Verification

### k3s (REST + gRPC over TLS)

```bash
# REST through the ingress (TLS on 443 via Traefik; plain HTTP on 80 also works)
curl --cacert ca.crt https://192.168.122.200/healthz
curl --cacert ca.crt https://100.118.225.119/healthz   # tailnet IP

# gRPC through the Traefik IngressRouteTCP (implicit TLS, no -plaintext)
grpcurl -cacert ca.crt 192.168.122.200:50051 list
grpcurl -cacert ca.crt -d '{"service":"OdbDesignService"}' 192.168.122.200:50051 \
    Odb.Grpc.OdbDesignService/HealthCheck

# Or after OS-level trust import, drop -cacert/--cacert entirely:
curl https://192.168.122.200/healthz
grpcurl 192.168.122.200:50051 list
```

`scripts/validate-grpc-exposure.ps1 -Tls [-CaCert ca.crt]` runs the same gRPC
checks through the TLS path.

### docker compose (deploy/nginx/)

The compose shape terminates TLS in nginx (bring-your-own cert, see
`deploy/nginx/README.md`); cert-manager is not involved there. Verification with a
cert issued from the same CA:

```bash
curl --cacert ca.crt https://<compose-host>/healthz
grpcurl -cacert ca.crt <compose-host>:443 list   # via nginx grpc_pass
```

## Notes

- Host-less ingress routing is preserved: `Ingress.tls.hosts` only accepts DNS
  names, so the `tls:` block lists only the `secretName`. Clients hitting bare IPs
  verify against the cert's IP SANs, not SNI.
- `/redoc` routes (`local-ingress.yaml`, `deploy/nginx/nginx.conf`) return 404 until
  the swaggerui sidecar image ships `redoc.html` (follow-up in the sidecar repo —
  M0.3's image half). The routing is already in place for both k3s and compose.
