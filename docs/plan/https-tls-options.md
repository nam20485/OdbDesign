# HTTPS/TLS strategy for OdbDesign deployments — design decision

| | |
|---|---|
| Status | **OPEN — no decision made yet.** This document enumerates the options and implementation outlines so a choice can be made later. |
| Date | 2026-08-21 |
| Scope | REST ingress TLS (all deployment targets), gRPC TLS (orthogonal, §8), repo layout for TLS config |
| Non-goals | Any vendor-specific default (e.g. Tailscale). OdbDesign is FOSS; other users deploy on their own infrastructure. The repo default must stay generic and the TLS layer must be pluggable per environment. |

## 1. Problem statement

Clients currently reach the REST API over `http://` or over `https://` with Traefik's
self-signed default cert (encrypts, but untrusted — every client must skip verification).
We want a documented, general way to serve **trusted** HTTPS on the ingress for the common
deployment environments: bare-metal/VM k3s or k3d, managed Kubernetes (EKS et al.), and
(private) networks without public DNS or public reachability.

## 2. Corrected history (why "it worked in k3d" is not quite right)

- `scripts/create-k3d-cluster.ps1` passes
  `--k3s-arg="--tls-san=<ip>,<hostname>@server:0"`. Those SANs go into the
  **kube-apiserver serving cert (port 6443)** — not the Traefik ingress. The current
  k3s setup replicates this exactly via `scripts/k3s-cluster.ps1 -TlsSans` (both
  `192.168.122.200` and `100.118.225.119` are baked into the API cert; kubeconfig works
  over TLS from anywhere that reaches those IPs).
- The only ingress-TLS config ever committed (`deploy/kube/issuer.yaml` + `tls:` in
  `local-ingress.yaml`, commit `8dd5fa2`, 2024-05-04) was **disabled the next day**
  (commit `4bc3110`, 2024-05-05). It could never have produced trusted certs anyway:
  it used the Let's Encrypt **staging** endpoint (deliberately untrusted), the ingress
  host was the bare hostname `precision5820` (ACME requires a FQDN in a public DNS
  zone; bare hostnames and IPs are ineligible), and cert-manager was never installed on
  any cluster (no install manifests exist in the repo).
- Consequently, the k3d cluster's published `8443:443` endpoint served Traefik's
  **default self-signed cert** (`CN=TRAEFIK DEFAULT CERT`) — the same thing the k3s
  cluster serves today. HTTPS "worked" only in the click-through-the-warning sense.
- The one TLS path that did ship is `deploy/nginx/` (docker compose): a
  bring-your-own-cert reverse proxy on `:443` covering REST, gRPC, and Swagger UI, with
  host-side `certbot certonly --standalone` documented in `deploy/nginx/README.md`.

**Takeaway:** nothing was lost in the k3s migration; trusted ingress TLS never existed
in any Kubernetes deployment of this repo. The `issuer.yaml` remnants should be treated
as a template, not a working reference.

## 3. Constraints and decision criteria

1. **Generic**: no dependency on a specific overlay network / VPN vendor. Others deploy
   this server on their own clusters.
2. **Covers both common shapes**:
   - public deployment (real domain, publicly reachable ingress) — e.g. the EKS target
     (`deploy/kube/default-ingress (eks).yaml`, internet-facing ALB);
   - private deployment (VM/LAN/VPN, IP addresses only, no public DNS) — e.g. the k3s VM.
3. **Renewal must be automatic** (ACME certs expire in 90 days) or explicitly
   low-frequency with an operator runbook (BYO corporate certs).
4. **No surprise client trust steps** where avoidable; where unavoidable (private CA),
   the CA distribution step must be documented.
5. REST first; gRPC TLS handled separately (§8) — gRPC is exposed via ServiceLB on
   50051, not through the HTTP ingress.
6. The repo default (what `deploy.ps1` applies with no extra flags) stays plain HTTP;
   TLS is layered on by an explicit overlay/flag per environment.

## 4. Option matrix

| # | Option | Trust | Needs public DNS? | Needs public reachability? | Works with IP-only clients? | Auto renewal | Extra controller |
|---|--------|-------|-------------------|---------------------------|------------------------------|--------------|------------------|
| A | cert-manager + ACME **HTTP-01** | Let's Encrypt (public) | Yes | Yes | No (hostnames only) | Yes | cert-manager |
| B | cert-manager + ACME **DNS-01** | Let's Encrypt (public) | Yes (+ DNS provider API) | **No** (fits private clusters) | No (hostnames only) | Yes | cert-manager |
| C | cert-manager + **private CA** Issuer | Own CA (distribute once) | No | No | **Yes** (IP SANs) | Yes (cert-manager re-issues) | cert-manager |
| D | **Traefik built-in ACME** (`certificatesResolvers`) | Let's Encrypt (public) | Yes | Yes (HTTP-01/TLS-01) | No | Yes | none (Traefik already shipped) |
| E | **Bring-your-own cert** (pre-made TLS Secret) | Whatever the operator brings (corporate CA, manually issued LE, etc.) | No | No | Yes (if SANs include IPs) | No — operator renews | none |
| F | Status quo: Traefik **default cert** | None (self-signed) | — | — | Yes | n/a (regenerated ~yearly) | none |

## 5. Options in detail

### Option A — cert-manager + Let's Encrypt via HTTP-01

**How it works.** cert-manager watches `Ingress` objects with the
`cert-manager.io/issuer` annotation and `spec.tls`. It solves the ACME HTTP-01
challenge by standing up a temporary pod/service that must be reachable at
`http://<domain>/.well-known/acme-challenge/...` through the ingress, then fills the
referenced TLS Secret; Traefik serves it. Renewal is automatic (~30 days before expiry).

**Prerequisites.** A FQDN you own, publicly resolvable to the ingress (A/ALIAS record);
ports 80/443 reachable from the Internet. Fits the EKS deployment shape naturally.

**Implementation outline.**
1. Install cert-manager (official static manifests, version-pinned):
   `kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.17.x/cert-manager.yaml`
   — or commit the vendored manifest under `deploy/kube/cert-manager/`.
2. Replace `deploy/kube/issuer.yaml` (staging) with a **production** `ClusterIssuer`:
   `acme.server: https://acme-v02.api.letsencrypt.org/directory`, `solvers: [{http01: {ingress: {class: traefik}}}]`
   (for EKS: `class: alb` or use the ALB's own ACM cert instead — see §7).
3. Un-comment/complete the `tls:` block in the overlay's ingress with the real hostname
   + `secretName: odbdesign-server-tls`, and set the `cert-manager.io/cluster-issuer` annotation.
4. Verification: `kubectl get certificate odbdesign-server-tls` → `READY: True`;
   `curl -sS https://<domain>/healthz` (no `-k`) → `200 ok`; `openssl s_client` shows the LE chain.
5. Keep staging issuer available as a test toggle (`-UseStagingIssuer` flag) to avoid
   LE rate limits while iterating.

**Notes.** Cannot issue for IPs or bare hostnames. On EKS, ACM + ALB (§7) usually makes
this option redundant for the cloud shape.

### Option B — cert-manager + Let's Encrypt via DNS-01

**How it works.** Same controller and Secret flow as A, but the challenge is solved by
writing a TXT record via the DNS provider's API (`_acme-challenge.<domain>`). **No
public reachability required** — the cluster can sit on a private network; only the
domain's DNS zone must be API-manageable. This is the standard answer for
"real cert, private cluster".

**Prerequisites.** FQDN in a DNS zone at a provider with an API cert-manager supports
out of the box — Cloudflare, Route53, Google Cloud DNS, Azure DNS, DigitalOcean,
deSEC, etc. (webhook/shell plugins extend to more). Provider API token stored as a
Secret. Public A record for the hostname is still needed for **clients** to resolve it,
but it may point at a private IP (split-horizon or simply a public record → private
address, e.g. `odbdesign.example.com A 100.118.225.119`); the issuance path ignores it.

**Implementation outline.**
1. cert-manager install as in A.1.
2. ClusterIssuer with a `dns01` solver per provider, e.g. Cloudflare:
   `solvers: [{dns01: {cloudDNS|cloudflare: {...},}}]` + token Secret
   (`deploy/kube/secrets/cloudflare-api-token.yaml`, git-ignored template committed).
3. Ingress `tls:` + annotation as in A.3.
4. Optional: a wildcard cert (`*.odbdesign.example.com`) so gRPC and any future
   services can share one issuance.
5. Verification as in A.4; additionally `kubectl describe challenge` when debugging,
   and confirm propagation with `dig TXT _acme-challenge.<domain>` during issuance.

**Notes.** The most common "general" solution for private Kubernetes clusters; token
handling is the only sensitive part. Cannot issue for bare IPs either.

### Option C — cert-manager with a self-managed CA (CA Issuer)

**How it works.** A `Certificate` + `Issuer` of type `CA` sign the ingress cert from a
private root/intermediate CA you generate once (e.g. `step-ca`, or plain `openssl`).
cert-manager handles issuance **and renewal of leaves** automatically. The **only**
option that can put IP SANs (`192.168.122.200`, `100.118.225.119`) and even local
hostnames into a cert, so `https://<ip>/` itself validates.

**Prerequisites.** None external. Fully offline. Clients must import the CA cert once
(curl `--cacert`, Linux `update-ca-certificates`, Windows cert store, JVM truststore,
`REQUESTS_CA_BUNDLE`, …). The CA private key lives in the cluster as a Secret.

**Implementation outline.**
1. Generate the CA (one-time, scripted in `scripts/make-tls-ca.ps1` or `.sh`):
   `openssl req -x509 -newkey ec -keyout ca.key -out ca.crt -days 3650 -subj "/CN=OdbDesign CA"`
   (or `step certificate create` if `step-ca` is preferred).
2. `kubectl create secret tls odbdesign-root-ca --cert=ca.crt --key=ca.key` (cluster-local;
   document that the root key may instead be kept offline and an intermediate used).
3. `deploy/kube/issuer-ca.yaml`: `Issuer` kind `CA`, `ca.secretName: odbdesign-root-ca`.
4. `deploy/kube/certificate-odbs-server.yaml`: `Certificate` with
   `dnsNames: [odbdesign.local, …]` and `ipAddresses: [192.168.122.200, 100.118.225.119]`,
   `secretName: odbdesign-server-tls`.
5. Ingress gets `tls: hosts/ip → secretName` (Ingress `tls.hosts` only takes DNS names;
   IP SANs still work for clients hitting the IP because Traefik presents the same cert —
   verification is against SANs, not SNI).
6. Publish `ca.crt` via the repo (`docs/` or a ConfigMap-served endpoint) + client
   import instructions per platform.
7. Verification: `openssl s_client -connect 192.168.122.200:443` chain resolves to the
   imported CA; `curl --cacert ca.crt https://192.168.122.200/healthz` → `200 ok`.

**Notes.** "Real" PKI hygiene without any external dependency. Best fit for the
private-VM shape and for consumers who cannot use public DNS at all. The
trust-distribution step is the trade-off.

### Option D — Traefik built-in Let's Encrypt (`certificatesResolvers`)

**How it works.** k3s already bundles Traefik; Traefik itself can talk ACME (HTTP-01 or
TLS-ALPN-01) and store the cert in a file/k8s Secret. No cert-manager deployment; one
dynamic-config stanza + ingress annotations
(`traefik.ingress.kubernetes.io/router.tls.certresolver: le`).

**Prerequisites.** Public DNS + reachability (same as A). Traefik configured via
`HelmChartConfig` in k3s (`/var/lib/rancher/k3s/server/manifests/traefik-config.yaml`).

**Implementation outline.**
1. Drop a `HelmChartConfig` for the packaged Traefik adding:
   `certificatesResolvers.le.acme.{email,storage,httpChallenge.entryPoint: web}`.
2. Annotate the OdbDesign ingress with the certresolver + entrypoint annotations and a
   `tls:` block (hostname required).
3. k3s applies it on next server start / `kubectl apply`; Traefik provisions on first
   HTTPS hit for the host.
4. Verification: `curl https://<domain>/healthz`; Traefik logs show ACME success.
5. Caveats: k3s manages the packaged Traefik — pin and document the HelmChartConfig;
   LE account/key stored in Traefik's config (add a PVC or accept re-issuance on restart).

**Notes.** Lightest-weight public-DNS option (no extra controller), slightly less
portable (Traefik-specific; EKS/ALB deployments can't use it) and fewer knobs than
cert-manager.

### Option E — Bring-your-own cert (pre-created TLS Secret)

**How it works.** Operator provisions any cert by any means (corporate PKI, manually
run `certbot`, CA from §C, etc.) and creates a `tls` Secret; the ingress references it
via `spec.tls.secretName`. Zero new moving parts in-cluster; mirrors the pattern already
shipped in `deploy/nginx/README.md` for the compose deployment.

**Prerequisites.** A cert with the right SANs from anywhere the operator trusts.

**Implementation outline.**
1. Commit an overlay `deploy/kube/overlays/tls-byoc/` containing the ingress with
   `tls: {hosts: […], secretName: odbdesign-server-tls}`.
2. Document/parametrize secret creation in `deploy.ps1`:
   `kubectl create secret tls odbdesign-server-tls --cert=fullchain.pem --key=privkey.pem`
   (script flag `-TlsCertDir <path>`; refuse to apply the overlay if the Secret is missing).
3. Runbook for renewal (re-run certbot / corporate reissue → `kubectl create secret …`
   again → Traefik picks it up; no restart needed).
4. Verification: `curl --cacert <issuer> https://<host>/healthz` → `200 ok`.

**Notes.** The escape hatch that makes every other option composable — any issuer
(including B or C run outside the cluster) can feed it. Not "automatic" by itself.

### Option F — Status quo (Traefik default self-signed cert)

Documented for completeness: `https://<ip>` encrypts with `CN=TRAEFIK DEFAULT CERT`,
clients must skip verification (`curl -k`, `InsecureSkipVerify`, browser warning).
Acceptable for lab/tailnet-only use; explicitly not acceptable as the production story.
**Action regardless of decision:** record this in the README/access map so it's a known
state, not a surprise.

## 6. Comparison against the constraints

| Criterion | A | B | C | D | E | F |
|---|---|---|---|---|---|---|
| Public deployment (EKS/domain) | ✅ | ✅ | ✅ | ✅ (Traefik only) | ✅ | ✅ (untrusted) |
| Private VM/IP-only (k3s) | ❌ | ⚠️ needs owned domain+API | ✅✅ | ❌ | ✅ | ✅ (untrusted) |
| Trusted by stock clients, zero setup | ✅ | ✅ | ❌ (import CA) | ✅ | depends | ❌ |
| Automatic renewal | ✅ | ✅ | ✅ (leaves) | ✅ | ❌ | n/a |
| No extra controller | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ |
| Generic / no vendor coupling | ✅ | ✅ | ✅ | ⚠️ Traefik-specific | ✅✅ | ✅ |

## 7. Special case: cloud (EKS/ALB) deployments

On AWS the idiomatic path bypasses in-cluster ACME entirely: **ACM certificate attached
to the ALB** (`alb.ingress.kubernetes.io/certificate-arn` annotation on
`deploy/kube/default-ingress (eks).yaml`) or an ACM-cert-backed listener. ACM renews
automatically; DNS validated once via the AWS console/CLI. If the project wants a
first-class cloud story, this is Option G in spirit (managedissuer variant) and costs
one annotation + docs. Keep it in the decision when the EKS target is revisited.

## 8. gRPC TLS (orthogonal, currently plaintext on 50051)

The gRPC Service is `type: LoadBalancer` (ServiceLB binds node :50051) — it does not
pass through the HTTP ingress, so ingress TLS changes nothing for it. Options, in
increasing order of effort:

1. **Traefik TCP termination**: replace the LoadBalancer service with a
   `TraefikService`/`IngressRouteTCP` (or annotate the Service for Traefik) publishing
   `:50051` with `tls:` — reuses whatever cert the ingress uses (A–E all feed it).
   grpcurl then uses `grpcurl <host>:50051 ...` (implicit TLS) instead of `-plaintext`.
2. **TLS passthrough**: same route with `tls.passthrough`, cert served by the app.
3. **App-native TLS**: Crow/gRPC server-side TLS (`CROW_ENABLE_SSL` build flag + cert
   config; gRPC credentials). Most portable (works on compose/nginx too) but touches
   the C++ build and deployment config.
4. **Continue plaintext on trusted networks** (status quo) and terminate at an
   edge proxy for public exposure — exactly what `deploy/nginx/` already implements
   for compose (`grpc_pass` + `:443`).

Decision for gRPC should follow the REST decision (same cert source); no separate
cert infrastructure.

## 9. Proposed repo layout (applies to whichever option(s) are chosen)

```
deploy/kube/
  issuer.yaml                      # replaced per decision (or deleted)
  overlays/
    tls-acme-http01/               # Option A
    tls-acme-dns01/                # Option B   (kustomize overlays:
    tls-private-ca/                # Option C    base = local-ingress.yaml +
    tls-byoc/                      # Option E    issuer/certificate/secret refs)
```

- `scripts/deploy.ps1` gains optional `-TlsOverlay <name>`; default remains plain HTTP
  (no overlay), preserving today's behavior.
- Base ingress stays host-less (bare-IP routing keeps working); overlays add
  host + tls. CA cert, provider tokens, and BYO cert files stay git-ignored with
  committed templates.

## 10. Open questions (to answer when deciding)

1. Do we commit to **one** recommended option for the README (and which), or publish
   overlays for all of A/B/C/E and let consumers pick?
2. Does the primary maintainer deployment (k3s VM, IP-based, no owned domain in play)
   need trusted-by-default clients at all — or is C (private CA, one-time import) the
   pragmatic pick for it while B is the documented "real domain" path?
3. cert-manager: vendored static manifest (pinned, offline-friendly) vs official helm
   chart values under `deploy/helm/` (consistent with monitoring stack)?
4. Scope gRPC TLS into the first implementation or defer (§8)?
5. Housekeeping either way: fix or remove the stale staging `issuer.yaml`, and document
   Option F's default-cert behavior in the access map.
