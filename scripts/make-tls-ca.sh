#!/usr/bin/env bash
#
# One-time generation of the OdbDesign private root CA (https-tls-options.md
# Option C: cert-manager + self-managed CA with IP SANs; server-issues
# implementation plan M3.1).
#
# Generates a 10-year EC (prime256v1) self-signed root certificate and key in
# the given output directory (default: current directory), hardened for modern
# clients: SHA-256+ only (no SHA-1), explicit CA basicConstraints/keyUsage.
#
# Usage:
#   ./scripts/make-tls-ca.sh [output-dir]
#
# After running, import the CA into the cluster (command is printed at the end):
#   kubectl create secret tls odbdesign-root-ca --cert=ca.crt --key=ca.key -n default
#
# Keep ca.key private. It may instead be kept offline with an intermediate CA
# signing the leaves — see deploy/kube/cert-manager/README.md.

set -euo pipefail

OUT_DIR="${1:-.}"
CA_KEY="$OUT_DIR/ca.key"
CA_CRT="$OUT_DIR/ca.crt"

if ! command -v openssl >/dev/null 2>&1; then
    echo "error: openssl not found in PATH" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# Refuse to clobber an existing CA: silently regenerating would change the
# trust anchor out from under every client that imported the old cert.
for f in "$CA_KEY" "$CA_CRT"; do
    if [[ -e "$f" ]]; then
        echo "error: $f already exists; move it aside or delete it explicitly to generate a new CA." >&2
        exit 1
    fi
done

# EC prime256v1 root, SHA-256 signatures only (never SHA-1), 10-year validity.
# -nodes keeps the key unencrypted so the script runs unattended (the file is
# written mode 600; it is imported into the cluster Secret or kept offline).
openssl req -x509 \
    -newkey ec \
    -pkeyopt ec_paramgen_curve:prime256v1 \
    -sha256 \
    -nodes \
    -keyout "$CA_KEY" \
    -out "$CA_CRT" \
    -days 3650 \
    -subj "/CN=OdbDesign CA" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" \
    -addext "subjectKeyIdentifier=hash"

chmod 600 "$CA_KEY"

echo
echo "Root CA generated:"
echo "  cert: $CA_CRT"
echo "  key:  $CA_KEY (mode 600 — keep private; may be kept offline, see README)"
echo
echo "Next step — import into the cluster so the cert-manager CA Issuer can sign leaves:"
echo
echo "  kubectl create secret tls odbdesign-root-ca --cert=$CA_CRT --key=$CA_KEY -n default"
echo
echo "Then distribute $CA_CRT to clients (curl --cacert, update-ca-certificates,"
echo "Windows cert store, REQUESTS_CA_BUNDLE, ...) — see deploy/kube/cert-manager/README.md."
