# OdbDesignServer — nginx reverse proxy

Routes a single TLS endpoint on `:443` to the OdbDesignServer (REST + gRPC)
and the bundled swagger-ui. Sits in front of the `server:` and `swagger-ui:`
services defined in `../../compose.yml`.

## Layout

```
Internet ──► nginx (:443, :80)
            ├─ /Odb.Grpc.OdbDesignService/*  ──► server:50051  (gRPC, HTTP/2)
            ├─ /Odb.Lib.Protobuf.OdbDesignApi/* ──► server:50051  (gRPC, HTTP/2)
            ├─ /healthz/*, /ready             ──► server:8888  (REST)
            ├─ /designs/*                     ──► server:8888  (REST)
            ├─ /filemodels/*                  ──► server:8888  (REST)
            ├─ /files/*                       ──► server:8888  (REST, file upload)
            └─ / (catch-all)                  ──► swagger-ui:8080
```

## Required certs

Place these in `./ssl/` at the repo root (mounted into the proxy container
at `/etc/nginx/ssl/`):

| File          | Purpose                                                  |
|---------------|----------------------------------------------------------|
| `cert.pem`    | Leaf certificate + chain (or `fullchain.pem` from LE)    |
| `cert.key`    | Private key matching `cert.pem`                          |
| `dhparam.pem` | Optional — DH params for DHE ciphers (slower handshake)  |

These files are git-ignored (`.dockerignore` already excludes `ssl/`).

Quick start with Let's Encrypt (external to compose, run on the host):

```bash
sudo certbot certonly --standalone -d your.host.example.com
sudo cp /etc/letsencrypt/live/your.host.example.com/fullchain.pem ssl/cert.pem
sudo cp /etc/letsencrypt/live/your.host.example.com/privkey.pem   ssl/cert.key
sudo chmod 0640 ssl/cert.key
```

## Endpoints after the proxy is up

| What                       | URL                                                            |
|----------------------------|----------------------------------------------------------------|
| Swagger UI                 | `https://<host>/`                                              |
| REST API                   | `https://<host>/designs/...`, `https://<host>/filemodels/...`  |
| Health probes              | `https://<host>/healthz`, `https://<host>/ready`               |
| gRPC (server-streaming)    | `grpcurl -insecure <host>:443 Odb.Grpc.OdbDesignService/...`   |

For local development, the `server:` and `swagger-ui:` containers also
expose `127.0.0.1:8888`, `127.0.0.1:50051`, and `127.0.0.1:8080`
loopback-only — useful for bypassing the proxy during debugging
(e.g. attaching `grpcurl` directly to the gRPC port).

## Customizing routes

- **Add a new REST route**: append another `location` block above the
  catch-all `location /`. Order doesn't matter — nginx uses longest-prefix
  match.
- **Expose a new gRPC service**: add a new `location` with
  `grpc_pass grpc://odbesign_grpc;`.
- **Tune timeouts**: per-route `grpc_*_timeout` and `proxy_*_timeout` values.
  The gRPC server's keepalive is 30s (set in `OdbDesignServer/config.json`).

## OpenAPI spec note

The swagger-ui image ships the OpenAPI 3 spec baked in (see
`nam20485/OdbDesignServer-SwaggerUI` repo, `development/spec` branch).
When using swagger-ui's "Try it out" feature, pick the `/` server entry
(relative URL, first in the `servers:` list of
`swagger/odbdesign-server-0.9-swagger.yaml`) to test through the proxy
without hardcoding a hostname.

## Disabling the proxy

If you want to go back to the bare 8888/50051/8080 publish, edit
`compose.yml`:

- Remove the entire `proxy:` service block.
- Change `ports: - 127.0.0.1:8888:8888` etc. back to `ports: - 8888:8888`.
- Restart with `docker compose up`.
