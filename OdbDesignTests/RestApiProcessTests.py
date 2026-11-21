#!/usr/bin/env python3
"""Real external-process HTTP acceptance tests for the OdbDesignServer REST API.

Every scenario in this file drives the REAL OdbDesignServer binary over real
HTTP (stdlib http.client) against a real filesystem (temp designs dir) and
asserts real outcomes: HTTP statuses, response bodies, ETag / Cache-Control
headers, sha256-verified stored bytes, process exit codes, and TCP socket
state. No string-presence-in-file checks are used as proof of any runtime
behavior; expected values are derived independently from the design archives
themselves (tar member listings + text record counts) at test time.

Stdlib only (subprocess + http.client + tarfile + hashlib). Registered as a
ctest named RestApiProcessTests.

Usage:
    python3 RestApiProcessTests.py --server /path/to/OdbDesignServer \
        [--test-data-dir /path/to/ODB_TEST_DATA_DIR] [--keep-logs]

Environment:
    ODB_TEST_DATA_DIR  directory containing sample_design.tgz and
                       designodb_rigidflex.tgz (same fixtures the gtest suite
                       uses); may also be passed via --test-data-dir.

Exit codes:
    0 = all scenarios passed
    1 = at least one scenario failed
    2 = harness usage error (bad --server, broken fixture checkout)
    3 = skipped: no design fixtures available (ODB_TEST_DATA_DIR unset;
        ctest SKIP_RETURN_CODE maps this to a SKIPPED test, matching the
        gtest fixtures' GTEST_SKIP-without-fixtures posture)
"""

import argparse
import base64
import hashlib
import http.client
import json
import os
import shutil
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import traceback

IS_POSIX = os.name == "posix"

SAMPLE_DESIGN = "sample_design"
SAMPLE_ARCHIVE = "sample_design.tgz"
RIGIDFLEX_DESIGN = "designodb_rigidflex"
RIGIDFLEX_ARCHIVE = "designodb_rigidflex.tgz"

MULTIPART_DESIGN = "multipart_rigidflex"
MULTIPART_FILENAME = "multipart_rigidflex.tgz"

# Exact product output strings of the health routes (HealthCheckController +
# OdbServerAppBase). Asserted byte-for-byte; they ARE the product behavior.
HEALTH_BODIES = {
    "/healthz/live": "healthy: live",
    "/healthz/ready": "healthy: ready",
    "/healthz/started": "healthy: started",
    "/healthz": "ok",
    "/ready": "ready",
}

CACHE_CONTROL_EXPECTED = "private, max-age=3600"
UPLOAD_CONTENT_TYPE = "application/octet-stream"
MULTIPART_PART_NAME = "file"

# F004 fail-closed exit code (Utils::ExitCode::PreServerRunFailed)
PRE_SERVER_RUN_FAILED_EXIT_CODE = 5

HTTP_TIMEOUT_DEFAULT = 60
HTTP_TIMEOUT_HEAVY = 300  # first parse of an archive runs inside a GET
STARTUP_TIMEOUT_S = 60
PROCESS_EXIT_TIMEOUT_S = 30

# Exit codes (see the module docstring)
EXIT_ALL_PASSED = 0
EXIT_SCENARIOS_FAILED = 1
EXIT_USAGE_ERROR = 2
EXIT_SKIPPED_NO_TEST_DATA = 3  # ctest SKIP_RETURN_CODE maps this to SKIPPED


class HarnessError(Exception):
    """Fatal harness (not scenario) problem: bad arguments, missing fixtures."""


class Response:
    def __init__(self, status, headers, body):
        self.status = status
        self.headers = headers  # dict, lower-cased keys
        self.body = body  # bytes

    def text(self):
        return self.body.decode("utf-8", errors="replace")

    def json(self):
        return json.loads(self.body.decode("utf-8"))


class Scenario:
    """Records pass/fail per check; keeps going to gather full evidence."""

    def __init__(self, name):
        self.name = name
        self.failures = []
        self.checks = 0

    def check(self, condition, detail):
        self.checks += 1
        if not condition:
            self.failures.append(detail)
            print("    FAIL: %s" % detail)
        return bool(condition)

    def check_eq(self, actual, expected, what):
        return self.check(
            actual == expected,
            "%s: expected %r, got %r" % (what, expected, actual),
        )

    def finalize(self):
        status = "PASS" if not self.failures else "FAIL"
        print("[%s] %s (%d checks)" % (status, self.name, self.checks))
        return not self.failures


# --------------------------------------------------------------------------
# HTTP client
# --------------------------------------------------------------------------

def basic_auth_header(username, password):
    payload = ("%s:%s" % (username, password)).encode("utf-8")
    return "Basic " + base64.b64encode(payload).decode("ascii")


def encode_path_segment(segment):
    """Percent-encode a single URL path segment.

    Everything outside the unreserved set is escaped, including '+' (the
    server's UrlEncoding::decode maps a literal '+' to a space, so a layer
    named comp_+_top must travel as comp_%2B_top).
    """
    safe = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-~"
    out = []
    for byte in segment.encode("utf-8"):
        ch = chr(byte)
        if ch in safe:
            out.append(ch)
        else:
            out.append("%%%02X" % byte)
    return "".join(out)


class ServerHandle:
    """A running OdbDesignServer process + a thin HTTP client for it."""

    def __init__(self, binary, designs_dir, workdir, disable_auth, creds,
                 log_path):
        self.binary = binary
        self.designs_dir = designs_dir
        self.workdir = workdir
        self.disable_auth = disable_auth
        self.creds = creds  # None = scrub credential env vars entirely
        self.log_path = log_path
        self.process = None
        self.rest_port = None
        self.grpc_port = None
        self._log_file = None

    # -- lifecycle ---------------------------------------------------------

    def start(self):
        self.rest_port = _free_port()
        self.grpc_port = _free_port()
        args = [
            self.binary,
            "--bind", "127.0.0.1",
            "--port", str(self.rest_port),
            "--grpc-port", str(self.grpc_port),
            "--designs-dir", _relative_designs_arg(self.designs_dir,
                                                   self.workdir),
        ]
        if self.disable_auth:
            args.append("--disable-authentication")
        env = _child_env(self.creds)
        self._log_file = open(self.log_path, "wb")
        # codeql[py/command-line-injection] test harness: argv is the ctest-provided --server ($<TARGET_FILE:OdbDesignServer>), validated in main().
        self.process = subprocess.Popen(args, cwd=self.workdir, env=env, stdout=self._log_file, stderr=subprocess.STDOUT, start_new_session=IS_POSIX)

    def wait_ready(self, timeout=STARTUP_TIMEOUT_S):
        deadline = time.monotonic() + timeout
        last_error = None
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise ServerStartupError(
                    "server exited during startup (code %s); log tail:\n%s"
                    % (self.process.returncode, self.log_tail()))
            try:
                resp = self.http("GET", "/healthz/live",
                                 timeout=HTTP_TIMEOUT_DEFAULT)
                if resp.status == 200:
                    return
                last_error = "/healthz/live returned %s" % resp.status
            except (OSError, http.client.HTTPException) as exc:
                last_error = exc
            time.sleep(0.25)
        raise ServerStartupError(
            "server not ready within %ss (last: %r); log tail:\n%s"
            % (timeout, last_error, self.log_tail()))

    def stop(self):
        if self.process is None:
            return
        if self.process.poll() is None:
            try:
                if IS_POSIX:
                    os.killpg(self.process.pid, 15)
                else:
                    self.process.terminate()
            except OSError:
                pass
            try:
                self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                try:
                    if IS_POSIX:
                        os.killpg(self.process.pid, 9)
                    else:
                        self.process.kill()
                except OSError:
                    pass
                self.process.wait(timeout=10)
        if self._log_file is not None:
            self._log_file.close()
            self._log_file = None

    def log_tail(self, lines=40):
        try:
            with open(self.log_path, "rb") as fh:
                data = fh.read().decode("utf-8", errors="replace")
            return "\n".join(data.splitlines()[-lines:])
        except OSError:
            return "<log unavailable>"

    # -- HTTP ---------------------------------------------------------------

    def http(self, method, path, body=None, headers=None, auth=None,
             timeout=HTTP_TIMEOUT_DEFAULT):
        if auth is not None:
            headers = dict(headers or {})
            headers["Authorization"] = basic_auth_header(*auth)
        conn = http.client.HTTPConnection("127.0.0.1", self.rest_port,
                                          timeout=timeout)
        try:
            conn.request(method, path, body=body, headers=headers or {})
            resp = conn.getresponse()
            resp_headers = {k.lower(): v for k, v in resp.getheaders()}
            data = resp.read()
            return Response(resp.status, resp_headers, data)
        finally:
            conn.close()


class ServerStartupError(Exception):
    pass


def _free_port():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]
    finally:
        sock.close()


def _relative_designs_arg(designs_dir, workdir):
    """--designs-dir must be relative (CommandLineArgs treats a leading '/'
    as a flag), so pass it relative to the server's cwd."""
    return os.path.relpath(designs_dir, workdir)


def _child_env(creds):
    """Child environment with auth semantics under our control.

    Environment=Testing so the debug-build/Local-environment authentication
    bypass (RequestAuthenticationBase) can never mask Basic auth behavior.
    Credentials are set or scrubbed explicitly per server.
    """
    env = dict(os.environ)
    env["Environment"] = "Testing"
    env.pop("ODBDESIGN_SERVER_REQUEST_USERNAME", None)
    env.pop("ODBDESIGN_SERVER_REQUEST_PASSWORD", None)
    if creds is not None:
        env["ODBDESIGN_SERVER_REQUEST_USERNAME"] = creds[0]
        env["ODBDESIGN_SERVER_REQUEST_PASSWORD"] = creds[1]
    return env


def port_refuses(port, attempts=5, delay=0.4):
    """True when TCP connect to 127.0.0.1:port is refused on every attempt."""
    for _ in range(attempts):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        try:
            sock.connect(("127.0.0.1", port))
            return False
        except OSError:
            pass
        finally:
            sock.close()
        time.sleep(delay)
    return True


def start_server(binary, designs_dir, workdir, disable_auth, creds, log_name):
    handle = ServerHandle(binary, designs_dir, workdir, disable_auth, creds,
                          os.path.join(workdir, log_name))
    handle.start()
    try:
        handle.wait_ready()
    except ServerStartupError:
        handle.stop()
        # The picked port may have been grabbed between our probe and the
        # server's bind; retry once with fresh ports before failing. The
        # retry is guarded the same way: a spawn/startup failure must never
        # escape with the retried server still running (it would survive as
        # a session-detached orphan holding ports).
        handle = ServerHandle(binary, designs_dir, workdir, disable_auth,
                              creds, os.path.join(workdir, log_name))
        try:
            handle.start()
            handle.wait_ready()
        except BaseException:
            handle.stop()
            raise
    return handle


def make_multipart(filename, payload):
    boundary = "OdbDesignProcessTestBoundary73510293"
    part = []
    part.append(("--%s\r\n" % boundary).encode("ascii"))
    part.append(
        ('Content-Disposition: form-data; name="%s"; filename="%s"\r\n'
         % (MULTIPART_PART_NAME, filename)).encode("utf-8"))
    part.append(
        ("Content-Type: %s\r\n" % UPLOAD_CONTENT_TYPE).encode("ascii"))
    part.append(b"\r\n")
    part.append(payload)
    part.append(b"\r\n")
    part.append(("--%s--\r\n" % boundary).encode("ascii"))
    body = b"".join(part)
    content_type = "multipart/form-data; boundary=%s" % boundary
    return content_type, body


# --------------------------------------------------------------------------
# Independent expectations, derived from the design archives themselves
# --------------------------------------------------------------------------

class ArchiveFacts:
    """Independent ground truth parsed out of the ODB++ archive with the
    Python stdlib (tar member listing + plain text record counting). Nothing
    here uses OdbDesign code: agreement between this parser and the server is
    the cross-check."""

    def __init__(self, archive_path):
        self.path = archive_path
        # codeql[py/path-injection] test harness: fixture path from the ctest-provided ODB_TEST_DATA_DIR, validated in main()/prepare_workdir().
        self.size = os.path.getsize(archive_path)
        # codeql[py/path-injection] test harness: fixture path from the ctest-provided ODB_TEST_DATA_DIR, validated in main()/prepare_workdir().
        with open(archive_path, "rb") as fh:
            self.sha256 = hashlib.sha256(fh.read()).hexdigest()

        self.steps = []            # dir names under <root>/steps/
        self.layers = {}           # step -> sorted layer dir names
        self.symbols = []          # dir names under <root>/symbols/
        self.netlists = {}         # step -> sorted netlist dir names
        self.cmp_counts = {}       # layer dir -> CMP record count
        self.cmp_names = set()     # compName (field 7) across both CMP layers
        self.part_names = set()    # partName (field 8)
        self.nets = []             # NET record names from eda/data (order kept)
        self.pkg_count = 0         # PKG records in eda/data
        self.matrix_step_names = []  # NAME= inside STEP { } blocks
        self.matrix_layer_count = 0  # LAYER { } blocks
        self.misc_job_name = None    # JOB_NAME= in misc/info
        self.symbol_feature_counts = {}  # symbol -> feature-line count
        self.stephdr_text = None

        with tarfile.open(archive_path, "r:gz") as tf:
            infos = tf.getmembers()
            dirs = [m for m in infos if m.isdir()]
            files = {m.name: m for m in infos if m.isfile()}
            roots = sorted({m.name.split("/")[0] for m in infos})
            self.root = roots[0]
            steps_prefix = self.root + "/steps/"

            for m in dirs:
                parts = m.name.split("/")
                # <root>/steps/<step>/
                if len(parts) == 3 and m.name.startswith(steps_prefix):
                    self.steps.append(parts[2])
                # <root>/steps/<step>/layers/<layer>/
                if (len(parts) == 5 and parts[1] == "steps"
                        and parts[3] == "layers"):
                    self.layers.setdefault(parts[2], []).append(parts[4])
                # <root>/steps/<step>/netlists/<netlist>/
                if (len(parts) == 5 and parts[1] == "steps"
                        and parts[3] == "netlists"):
                    self.netlists.setdefault(parts[2], []).append(parts[4])
                # <root>/symbols/<symbol>/
                if len(parts) == 3 and parts[1] == "symbols":
                    self.symbols.append(parts[2])

            self.steps.sort()
            for step in self.layers:
                self.layers[step].sort()
            for step in self.netlists:
                self.netlists[step].sort()
            self.symbols.sort()

            # CMP / part records per components layer file
            for step, layer_list in self.layers.items():
                for layer in layer_list:
                    member = "%s/steps/%s/layers/%s/components" % (
                        self.root, step, layer)
                    if member in files:
                        text = self._read(tf, files[member])
                        for line in text.splitlines():
                            if line.startswith("CMP "):
                                fields = line.split()
                                if len(fields) >= 8:
                                    self.cmp_counts[layer] = \
                                        self.cmp_counts.get(layer, 0) + 1
                                    self.cmp_names.add(fields[6])
                                    self.part_names.add(fields[7])

            # EDA data: nets + packages of the alphabetically-first step
            # (Design::BuildNets reads steps.begin(), i.e. map order).
            if self.steps:
                first = self.steps[0]
                member = "%s/steps/%s/eda/data" % (self.root, first)
                if member in files:
                    text = self._read(tf, files[member])
                    for line in text.splitlines():
                        if line.startswith("NET "):
                            name = line.split()[1]
                            if name not in self.nets:
                                self.nets.append(name)
                        elif line.startswith("PKG "):
                            self.pkg_count += 1

            # matrix blocks
            member = "%s/matrix/matrix" % self.root
            if member in files:
                text = self._read(tf, files[member])
                current_block = None
                for line in text.splitlines():
                    stripped = line.strip()
                    if stripped.startswith("STEP {"):
                        current_block = "STEP"
                    elif stripped.startswith("LAYER {"):
                        current_block = "LAYER"
                        self.matrix_layer_count += 1
                    elif stripped == "}":
                        current_block = None
                    elif current_block == "STEP" and stripped.startswith(
                            "NAME="):
                        self.matrix_step_names.append(
                            stripped[len("NAME="):].strip())

            # misc/info JOB_NAME
            member = "%s/misc/info" % self.root
            if member in files:
                for line in self._read(tf, files[member]).splitlines():
                    if line.startswith("JOB_NAME="):
                        self.misc_job_name = line[len("JOB_NAME="):].strip()

            # symbol feature counts (fixture symbols use simple L records)
            for symbol in self.symbols:
                member = "%s/symbols/%s/features" % (self.root, symbol)
                if member in files:
                    count = 0
                    for line in self._read(tf, files[member]).splitlines():
                        # feature geometry records; comment lines start '#'
                        if line and not line.startswith("#"):
                            if line[0] in "LPA":
                                count += 1
                    self.symbol_feature_counts[symbol] = count

            member = "%s/steps/%s/stephdr" % (self.root, self.steps[0]) \
                if self.steps else None
            if member and member in files:
                self.stephdr_text = self._read(tf, files[member])

    @staticmethod
    def _read(tf, member):
        fh = tf.extractfile(member)
        if fh is None:
            return ""
        return fh.read().decode("utf-8", errors="replace")


# --------------------------------------------------------------------------
# Scenario context
# --------------------------------------------------------------------------

class Context:
    def __init__(self, server, designs_dir, facts_sample, facts_rigidflex,
                 replacement_path):
        self.server = server
        self.designs_dir = designs_dir
        self.facts = {SAMPLE_DESIGN: facts_sample,
                      RIGIDFLEX_DESIGN: facts_rigidflex}
        self.replacement_path = replacement_path

    def designs_dir_listing(self):
        return sorted(os.listdir(self.designs_dir))


# --------------------------------------------------------------------------
# Scenarios (server B: --disable-authentication unless noted)
# --------------------------------------------------------------------------

def scenario_health_endpoints(ctx, sc):
    """All five health routes answer 200 with their exact product bodies."""
    for path, expected_body in sorted(HEALTH_BODIES.items()):
        resp = ctx.server.http("GET", path)
        if sc.check_eq(resp.status, 200, "%s status" % path):
            sc.check_eq(resp.text(), expected_body, "%s body" % path)


def scenario_design_list_empty_is_204(ctx, sc):
    """With an empty designs dir the three list routes answer 204 No Content."""
    for path in ("/designs", "/files/list", "/filemodels"):
        resp = ctx.server.http("GET", path)
        sc.check_eq(resp.status, 204, "%s on empty designs dir" % path)


def scenario_octet_stream_upload_stores_exact_bytes(ctx, sc):
    """POST /files/upload/{name} (octet-stream) -> 201 and the stored file
    on disk is byte-identical to the uploaded archive (sha256)."""
    facts = ctx.facts[SAMPLE_DESIGN]
    payload = open(facts.path, "rb").read()
    path = "/files/upload/" + encode_path_segment(SAMPLE_ARCHIVE)
    resp = ctx.server.http(
        "POST", path, body=payload,
        headers={"Content-Type": UPLOAD_CONTENT_TYPE},
        timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 201, "octet-stream upload status"):
        sc.check(False, "upload response body: %r" % resp.text()[:300])
        return
    stored = os.path.join(ctx.designs_dir, SAMPLE_ARCHIVE)
    with open(stored, "rb") as fh:
        stored_sha = hashlib.sha256(fh.read()).hexdigest()
    sc.check_eq(stored_sha, facts.sha256,
                "stored %s bytes round-trip (sha256)" % SAMPLE_ARCHIVE)
    sc.check_eq(os.path.getsize(stored), facts.size, "stored size")


def scenario_multipart_upload_stores_exact_bytes(ctx, sc):
    """POST /files/upload (multipart/form-data) -> 201 and the stored file
    is byte-identical (sha256) to the uploaded archive."""
    facts = ctx.facts[RIGIDFLEX_DESIGN]
    payload = open(facts.path, "rb").read()
    content_type, body = make_multipart(MULTIPART_FILENAME, payload)
    resp = ctx.server.http(
        "POST", "/files/upload", body=body,
        headers={"Content-Type": content_type},
        timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 201, "multipart upload status"):
        sc.check(False, "multipart response body: %r" % resp.text()[:300])
        return
    stored = os.path.join(ctx.designs_dir, MULTIPART_FILENAME)
    with open(stored, "rb") as fh:
        stored_sha = hashlib.sha256(fh.read()).hexdigest()
    sc.check_eq(stored_sha, facts.sha256,
                "stored %s bytes round-trip (sha256)" % MULTIPART_FILENAME)


def scenario_unsupported_content_type_rejected(ctx, sc):
    """Both upload endpoints reject non-matching Content-Type with 400 and
    store nothing."""
    before = ctx.designs_dir_listing()
    resp = ctx.server.http(
        "POST",
        "/files/upload/" + encode_path_segment("wrongtype.tgz"),
        body=b"irrelevant",
        headers={"Content-Type": "text/plain"})
    sc.check_eq(resp.status, 400,
                "octet-stream route with text/plain Content-Type")
    content_type, body = make_multipart("x.tgz", b"irrelevant")
    resp = ctx.server.http(
        "POST", "/files/upload", body=body,
        headers={"Content-Type": UPLOAD_CONTENT_TYPE})
    sc.check_eq(resp.status, 400,
                "multipart route with octet-stream Content-Type")
    sc.check_eq(ctx.designs_dir_listing(), before,
                "no files stored by rejected content types")


def scenario_duplicate_octet_upload_conflict_keeps_original(ctx, sc):
    """Re-uploading an existing octet-stream name -> 409 Conflict and the
    original stored bytes are untouched (sha256 re-verified)."""
    original_path = os.path.join(ctx.designs_dir, SAMPLE_ARCHIVE)
    with open(original_path, "rb") as fh:
        original_sha = hashlib.sha256(fh.read()).hexdigest()
    different = b"NOT the original archive content at all" * 100
    resp = ctx.server.http(
        "POST", "/files/upload/" + encode_path_segment(SAMPLE_ARCHIVE),
        body=different, headers={"Content-Type": UPLOAD_CONTENT_TYPE})
    sc.check_eq(resp.status, 409, "duplicate octet-stream upload status")
    with open(original_path, "rb") as fh:
        after_sha = hashlib.sha256(fh.read()).hexdigest()
    sc.check_eq(after_sha, original_sha,
                "original bytes unchanged after 409")


def scenario_duplicate_multipart_upload_conflict_keeps_original(ctx, sc):
    """Re-uploading an existing multipart filename -> 409 and the original
    stored bytes are untouched."""
    original_path = os.path.join(ctx.designs_dir, MULTIPART_FILENAME)
    with open(original_path, "rb") as fh:
        original_sha = hashlib.sha256(fh.read()).hexdigest()
    different = b"second attempt with different payload" * 100
    content_type, body = make_multipart(MULTIPART_FILENAME, different)
    resp = ctx.server.http("POST", "/files/upload", body=body,
                           headers={"Content-Type": content_type})
    sc.check_eq(resp.status, 409, "duplicate multipart upload status")
    with open(original_path, "rb") as fh:
        after_sha = hashlib.sha256(fh.read()).hexdigest()
    sc.check_eq(after_sha, original_sha,
                "multipart original bytes unchanged after 409")


def scenario_hostile_upload_names_rejected(ctx, sc):
    """Traversal / absolute / control-character filenames are rejected with
    400 and NOTHING is written anywhere on the filesystem."""
    hostile_names = [
        ("../evil.tgz", "parent traversal"),
        ("a/../../evil.tgz", "multi-level traversal"),
        ("/tmp/odbdesign-hostile-escape.tgz", "absolute path"),
        ("..\\..\\evil.tgz", "windows backslash traversal"),
        ("bad\x01name.tgz", "control character"),
        ("trailingdot.tgz.", "trailing dot"),
    ]
    before = ctx.designs_dir_listing()
    escape_targets = [
        os.path.join(ctx.designs_dir, "evil.tgz"),
        os.path.join(os.path.dirname(ctx.designs_dir), "evil.tgz"),
        os.path.join(os.path.dirname(os.path.dirname(ctx.designs_dir)),
                     "evil.tgz"),
        "/tmp/odbdesign-hostile-escape.tgz",
    ]
    for raw_name, label in hostile_names:
        resp = ctx.server.http(
            "POST", "/files/upload/" + encode_path_segment(raw_name),
            body=b"malicious payload",
            headers={"Content-Type": UPLOAD_CONTENT_TYPE})
        sc.check_eq(resp.status, 400, "hostile name (%s) rejected" % label)
    sc.check_eq(ctx.designs_dir_listing(), before,
                "designs dir unchanged after hostile uploads")
    for target in escape_targets:
        sc.check(not os.path.exists(target),
                 "no file escaped to %s" % target)


def scenario_design_list_populated(ctx, sc):
    """After uploads, the list routes report both design names with the
    documented entry shape."""
    expected_names = {SAMPLE_DESIGN, MULTIPART_DESIGN}
    resp = ctx.server.http("GET", "/designs")
    if not sc.check_eq(resp.status, 200, "GET /designs populated status"):
        return
    body = resp.json()
    entries = body.get("filearchives")
    if not sc.check(isinstance(entries, list) and len(entries) > 0,
                    "GET /designs body carries a non-empty filearchives list"):
        return
    listed_names = {e.get("name") for e in entries}
    sc.check(expected_names <= listed_names,
             "GET /designs lists uploaded designs %r (got %r)"
             % (sorted(expected_names), sorted(listed_names)))
    for entry in entries:
        sc.check(isinstance(entry.get("loaded"), bool),
                 "entry %r has boolean 'loaded'" % entry.get("name"))
        sc.check(entry.get("type") in ("FileArchive", "Design"),
                 "entry %r has known 'type'" % entry.get("name"))
    for path in ("/files/list", "/filemodels"):
        resp2 = ctx.server.http("GET", path)
        sc.check_eq(resp2.status, 200, "%s populated status" % path)


def scenario_filemodel_route_matrix(ctx, sc):
    """FileModel route matrix for sample_design: every returned collection
    matches values parsed independently from the archive."""
    facts = ctx.facts[SAMPLE_DESIGN]
    step = facts.steps[0]
    step_seg = encode_path_segment(step)
    base = "/filemodels/" + encode_path_segment(SAMPLE_DESIGN)

    # steps list == archive step directory names
    resp = ctx.server.http("GET", base + "/steps")
    if sc.check_eq(resp.status, 200, "GET steps status"):
        sc.check_eq(resp.json().get("steps"), facts.steps,
                    "steps list matches archive step directories")

    # step document
    resp = ctx.server.http("GET", "%s/steps/%s" % (base, step_seg),
                           timeout=HTTP_TIMEOUT_HEAVY)
    sc.check_eq(resp.status, 200, "GET steps/<step> status")

    # layers list == archive layer directory names
    resp = ctx.server.http("GET", "%s/steps/%s/layers" % (base, step_seg))
    if sc.check_eq(resp.status, 200, "GET layers status"):
        sc.check_eq(resp.json().get("layers"), facts.layers[step],
                    "layers list matches archive layer directories")

    # one real layer document (the '+' in the name forces %2B encoding)
    layer_with_plus = next(l for l in facts.layers[step] if "+" in l)
    resp = ctx.server.http(
        "GET", "%s/steps/%s/layers/%s"
        % (base, step_seg, encode_path_segment(layer_with_plus)))
    sc.check_eq(resp.status, 200, "GET layers/<layer +%s> status"
                % layer_with_plus)

    # per-layer features: numFeatures agrees with the record list length
    paste_layer = next(l for l in facts.layers[step] if "paste" in l.lower())
    resp = ctx.server.http(
        "GET", "%s/steps/%s/layers/%s/features"
        % (base, step_seg, encode_path_segment(paste_layer)),
        timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET layer features status"):
        body = resp.json()
        records = body.get("featureRecords")
        sc.check(isinstance(records, list) and len(records) > 0,
                 "layer features carry a non-empty featureRecords list")
        sc.check_eq(body.get("numFeatures"),
                    len(records) if isinstance(records, list) else None,
                    "layer features numFeatures == featureRecords length")

    # per-layer components file: record count == archive CMP record count
    comp_layer = next(l for l in facts.cmp_counts
                      if facts.cmp_counts[l] == max(facts.cmp_counts.values()))
    resp = ctx.server.http(
        "GET", "%s/steps/%s/layers/%s/components"
        % (base, step_seg, encode_path_segment(comp_layer)),
        timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET layer components status"):
        body = resp.json()
        records = body.get("componentRecords")
        sc.check_eq(len(records) if isinstance(records, list) else -1,
                    facts.cmp_counts[comp_layer],
                    "layer components count matches archive CMP records "
                    "for %s" % comp_layer)
        if isinstance(records, list) and records:
            first = records[0]
            sc.check(first.get("compName") in facts.cmp_names,
                     "first component compName exists in archive CMP records")

    # eda_data: net record count == archive NET records, first net preserved
    resp = ctx.server.http("GET", "%s/steps/%s/eda_data" % (base, step_seg),
                           timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET eda_data status"):
        body = resp.json()
        net_records = body.get("netRecords")
        sc.check_eq(len(net_records) if isinstance(net_records, list) else -1,
                    len(facts.nets),
                    "eda_data net record count matches archive NET records")
        if isinstance(net_records, list) and net_records:
            sc.check_eq(net_records[0].get("name"), facts.nets[0],
                        "first eda net name matches archive (%s)"
                        % facts.nets[0])

    # netlists: list == archive netlist dirs; document counts match archive
    resp = ctx.server.http("GET", "%s/steps/%s/netlists" % (base, step_seg))
    if sc.check_eq(resp.status, 200, "GET netlists list status"):
        sc.check_eq(resp.json().get("netlists"), facts.netlists.get(step, []),
                    "netlists list matches archive netlist directories")
    netlist = facts.netlists.get(step, [None])[0]
    if netlist:
        resp = ctx.server.http(
            "GET", "%s/steps/%s/netlists/%s"
            % (base, step_seg, encode_path_segment(netlist)),
            timeout=HTTP_TIMEOUT_HEAVY)
        if sc.check_eq(resp.status, 200, "GET netlists/<name> status"):
            body = resp.json()
            sc.check(isinstance(body.get("netRecordss"), list)
                     and len(body["netRecordss"]) > 0,
                     "netlist document carries net records "
                     "(server contract key 'netRecordss')")
            sc.check(isinstance(body.get("netPointRecords"), list)
                     and len(body["netPointRecords"]) > 0,
                     "netlist document carries net point records")

    # symbols: list == archive symbol dirs; one symbol's features count
    resp = ctx.server.http("GET", base + "/symbols")
    if sc.check_eq(resp.status, 200, "GET symbols list status"):
        sc.check_eq(resp.json().get("symbols"), facts.symbols,
                    "symbols list matches archive symbol directories")
    if facts.symbols:
        symbol = facts.symbols[0]
        resp = ctx.server.http(
            "GET", "%s/symbols/%s/features"
            % (base, encode_path_segment(symbol)), timeout=HTTP_TIMEOUT_HEAVY)
        if sc.check_eq(resp.status, 200, "GET symbols/<name>/features status"):
            body = resp.json()
            sc.check_eq(body.get("numFeatures"),
                        facts.symbol_feature_counts.get(symbol),
                        "symbol %s numFeatures matches archive feature "
                        "records" % symbol)

    # matrix: STEP/LAYER blocks from the archive matrix file
    resp = ctx.server.http("GET", base + "/matrix/matrix",
                           timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET matrix status"):
        body = resp.json()
        matrix_steps = body.get("steps")
        sc.check(isinstance(matrix_steps, list) and len(matrix_steps) > 0,
                 "matrix document carries steps")
        if isinstance(matrix_steps, list) and matrix_steps:
            sc.check_eq([s.get("name") for s in matrix_steps],
                        facts.matrix_step_names,
                        "matrix step names match archive STEP blocks")
        sc.check_eq(len(body.get("layers", [])), facts.matrix_layer_count,
                    "matrix layer count matches archive LAYER blocks")

    # misc/info: JOB_NAME from the archive
    resp = ctx.server.http("GET", base + "/misc/info")
    if sc.check_eq(resp.status, 200, "GET misc/info status"):
        sc.check_eq(resp.json().get("jobName"), facts.misc_job_name,
                    "misc/info jobName matches archive JOB_NAME")

    # misc/attrlist + step attrlist + step profile: real parsed documents
    for suffix in ("/misc/attrlist", "/steps/%s/attrlist" % step_seg,
                   "/steps/%s/profile" % step_seg,
                   "/steps/%s/stephdr" % step_seg):
        resp = ctx.server.http("GET", base + suffix,
                               timeout=HTTP_TIMEOUT_HEAVY)
        sc.check_eq(resp.status, 200, "GET %s status" % suffix)

    # stephdr: the archive's own scalar values come back
    if facts.stephdr_text:
        def _stephdr_value(key):
            for line in facts.stephdr_text.splitlines():
                if line.startswith(key + "="):
                    return line[len(key) + 1:].strip()
            return None
        resp = ctx.server.http("GET", "%s/steps/%s/stephdr" % (base, step_seg))
        if sc.check_eq(resp.status, 200, "GET stephdr (values) status"):
            body = resp.json()
            sc.check_eq(body.get("xDatum"),
                        _to_number(_stephdr_value("X_DATUM")),
                        "stephdr xDatum matches archive X_DATUM")
            sc.check_eq(body.get("affectingBom"),
                        _stephdr_value("AFFECTING_BOM"),
                        "stephdr affectingBom matches archive AFFECTING_BOM")

    # diagnostics/symbol_units: per-layer diagnostics for exactly the
    # archive's layer set
    resp = ctx.server.http("GET", "%s/steps/%s/diagnostics/symbol_units"
                           % (base, step_seg), timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET diagnostics/symbol_units status"):
        body = resp.json()
        sc.check_eq(body.get("design"), SAMPLE_DESIGN,
                    "diagnostics design name")
        sc.check_eq(body.get("step"), step, "diagnostics step name")
        diag_layers = body.get("layers")
        if sc.check(isinstance(diag_layers, list),
                    "diagnostics carries a layers list"):
            sc.check_eq(sorted(l.get("layer") for l in diag_layers),
                        facts.layers[step],
                        "diagnostics covers exactly the archive layers")
            for entry in diag_layers:
                sc.check(isinstance(entry.get("has_mixed_symbol_unit_types"),
                                    bool),
                         "diagnostics layer %r has boolean mixed-unit flag"
                         % entry.get("layer"))
                sc.check(isinstance(entry.get("symbols_total"), int),
                         "diagnostics layer %r has symbol count"
                         % entry.get("layer"))

    # fonts/standard: real parsed font document
    resp = ctx.server.http("GET", base + "/fonts/standard",
                           timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET fonts/standard status"):
        body = resp.json()
        blocks = body.get("mCharacterBlocks")
        sc.check(isinstance(blocks, list) and len(blocks) > 0,
                 "fonts/standard carries character blocks (server contract "
                 "key 'mCharacterBlocks')")

    # 404 matrix: every missing entity answers NOT_FOUND
    missing_cases = [
        ("/filemodels/" + encode_path_segment("ghost_design"),
         "missing design"),
        ("%s/steps/%s" % (base, encode_path_segment("ghost_step")),
         "missing step"),
        ("%s/steps/%s/layers/%s"
         % (base, step_seg, encode_path_segment("ghost_layer")),
         "missing layer"),
        ("%s/steps/%s/netlists/%s"
         % (base, step_seg, encode_path_segment("ghost_netlist")),
         "missing netlist"),
        ("%s/symbols/%s" % (base, encode_path_segment("ghost_symbol")),
         "missing symbol"),
        ("/designs/" + encode_path_segment("ghost_design"),
         "missing design (product model)"),
    ]
    for path, label in missing_cases:
        resp = ctx.server.http("GET", path)
        sc.check_eq(resp.status, 404, "GET %s -> %s" % (label, path))


def scenario_product_model_routes(ctx, sc):
    """ProductModel routes for sample_design: counts and names cross-checked
    against the archive; load endpoint states."""
    facts = ctx.facts[SAMPLE_DESIGN]
    base = "/designs/" + encode_path_segment(SAMPLE_DESIGN)

    resp = ctx.server.http("GET", base, timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 200, "GET /designs/<name> status"):
        return
    body = resp.json()
    # KNOWN DISCREPANCY (finding, pinned as real behavior): the swagger spec
    # documents Design.name as the design name, but the REST load path never
    # populates it (Design.cpp leaves m_name unset on the file-model build
    # path), so the document carries an empty name. Counts and name sets below
    # are the real identity assertions; this pins today's behavior so any
    # change is deliberate.
    sc.check_eq(body.get("name"), "", "design document name (currently "
                                      "empty on the REST path; see PR "
                                      "findings)")

    # default (clipped) response must not embed the file model
    sc.check("fileModel" not in body,
             "default design response omits the file model (clipped view)")

    # include_filearchive=true serves the full payload including fileModel
    resp_full = ctx.server.http(
        "GET", base + "?include_filearchive=true", timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp_full.status, 200,
                   "GET /designs/<name>?include_filearchive=true status"):
        sc.check(isinstance(resp_full.json().get("fileModel"), dict),
                 "include_filearchive=true response embeds the fileModel "
                 "document")

    # nets: names + count from the archive eda data
    resp = ctx.server.http("GET", base + "/nets", timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET /designs/<name>/nets status"):
        nets = resp.json()
        sc.check(isinstance(nets, list), "/nets body is a JSON array")
        sc.check_eq(len(nets), len(facts.nets),
                    "nets count matches archive NET records")
        net_names = [n.get("name") for n in nets]
        sc.check_eq(sorted(net_names), sorted(facts.nets),
                    "net name set matches archive NET records")
        sc.check(facts.nets[0] in net_names,
                 "$NONE$-style first net %r present" % facts.nets[0])

    # components: top + bottom CMP records aggregated
    resp = ctx.server.http("GET", base + "/components",
                           timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET /designs/<name>/components status"):
        components = resp.json()
        total_cmp = sum(facts.cmp_counts.values())
        sc.check_eq(len(components), total_cmp,
                    "components count matches archive CMP records (%d+%d)"
                    % (facts.cmp_counts.get("comp_+_top", 0),
                       facts.cmp_counts.get("comp_+_bot", 0)))
        comp_names = {c.get("refDes") for c in components}
        sc.check(bool(comp_names & facts.cmp_names),
                 "component refDes values intersect archive CMP compName "
                 "records")

    # packages: PKG record count from eda data
    resp = ctx.server.http("GET", base + "/packages",
                           timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET /designs/<name>/packages status"):
        packages = resp.json()
        sc.check_eq(len(packages), facts.pkg_count,
                    "packages count matches archive PKG records")
        if packages:
            sc.check(isinstance(packages[0].get("name"), str),
                     "first package has a name")

    # parts: distinct CMP part names from the archive
    resp = ctx.server.http("GET", base + "/parts", timeout=HTTP_TIMEOUT_HEAVY)
    if sc.check_eq(resp.status, 200, "GET /designs/<name>/parts status"):
        parts = resp.json()
        sc.check_eq(len(parts), len(facts.part_names),
                    "parts count matches distinct archive CMP part names")

    # load endpoint: accepted with a real status payload
    resp = ctx.server.http("POST", base + "/load")
    if sc.check_eq(resp.status, 202, "POST /designs/<name>/load status"):
        body = resp.json()
        sc.check_eq(body.get("designName"), SAMPLE_DESIGN,
                    "load response designName")
        sc.check(isinstance(body.get("status"), str)
                 and len(body["status"]) > 0,
                 "load response carries a status string")

    # load endpoint for a missing design: still 202, body says not_found
    resp = ctx.server.http(
        "POST", "/designs/" + encode_path_segment("ghost_design") + "/load")
    if sc.check_eq(resp.status, 202, "POST load for missing design status"):
        sc.check_eq(resp.json().get("status"), "not_found",
                    "load response status for missing design")


def scenario_conditional_get_etag_flow(ctx, sc):
    """Real RFC 7232 flow on /filemodels/multipart_rigidflex (the design
    uploaded via multipart): 200 + ETag -> If-None-Match 304 -> archive
    replaced via upload -> fresh 200 with a NEW ETag and the replaced
    content (verified via /steps after the background re-parse)."""
    facts = ctx.facts[SAMPLE_DESIGN]  # replacement content: sample archive
    rigid_base = "/filemodels/" + encode_path_segment(MULTIPART_DESIGN)
    stored_name = MULTIPART_FILENAME

    # 1. initial 200 with ETag + Cache-Control (forces the full parse first)
    resp = ctx.server.http("GET", rigid_base, timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 200, "initial GET /filemodels status"):
        return
    etag1 = resp.headers.get("etag")
    sc.check(isinstance(etag1, str) and len(etag1) > 2 and
             etag1.startswith('"'),
             "initial response carries a quoted ETag (got %r)" % etag1)
    sc.check_eq(resp.headers.get("cache-control"), CACHE_CONTROL_EXPECTED,
                "initial response Cache-Control")

    # 2. matching If-None-Match -> 304, ETag echoed, Cache-Control echoed
    resp = ctx.server.http("GET", rigid_base,
                           headers={"If-None-Match": etag1})
    if sc.check_eq(resp.status, 304, "If-None-Match matching tag -> 304"):
        sc.check_eq(resp.headers.get("etag"), etag1, "304 echoes ETag")
        sc.check_eq(resp.headers.get("cache-control"),
                    CACHE_CONTROL_EXPECTED, "304 echoes Cache-Control")

    # 3. weak form and list form also revalidate to 304
    resp = ctx.server.http("GET", rigid_base,
                           headers={"If-None-Match": "W/" + etag1})
    sc.check_eq(resp.status, 304, "If-None-Match weak form -> 304")
    resp = ctx.server.http("GET", rigid_base,
                           headers={"If-None-Match": '"other", %s' % etag1})
    sc.check_eq(resp.status, 304, "If-None-Match list form -> 304")

    # 4. non-matching tag -> fresh 200 (full serialization: heavy timeout)
    resp = ctx.server.http("GET", rigid_base,
                           headers={"If-None-Match": '"stale-tag-000"'},
                           timeout=HTTP_TIMEOUT_HEAVY)
    sc.check_eq(resp.status, 200, "If-None-Match non-matching tag -> 200")

    # 5. rotate: remove the stored archive, upload a rootless repackaging of
    #    the sample archive under the same name (201 through the real upload
    #    route), which must invalidate the cache and rotate the validator.
    #    Rootless = deterministic re-parse after replacement on every
    #    platform; see build_rootless_archive for the stale-extraction-tree
    #    production finding this sidesteps.
    stored = os.path.join(ctx.designs_dir, stored_name)
    os.remove(stored)
    payload = open(ctx.replacement_path, "rb").read()
    resp = ctx.server.http(
        "POST", "/files/upload/" + encode_path_segment(stored_name),
        body=payload, headers={"Content-Type": UPLOAD_CONTENT_TYPE},
        timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 201, "replacement upload status"):
        return
    # REAL filesystem effect: the stored file is exactly the replacement
    # payload (sha256), not the old archive under the same name.
    with open(stored, "rb") as fh:
        stored_sha = hashlib.sha256(fh.read()).hexdigest()
    sc.check_eq(stored_sha, hashlib.sha256(payload).hexdigest(),
                "stored bytes after replacement are the uploaded payload")

    # 6. old validator now revalidates to a fresh 200 with a NEW ETag
    resp = ctx.server.http("GET", rigid_base,
                           headers={"If-None-Match": etag1},
                           timeout=HTTP_TIMEOUT_HEAVY)
    if not sc.check_eq(resp.status, 200,
                       "stale If-None-Match after rotation -> fresh 200"):
        return
    etag2 = resp.headers.get("etag")
    sc.check(isinstance(etag2, str) and etag2 != etag1,
             "post-rotation ETag differs from pre-rotation (old %r new %r)"
             % (etag1, etag2))

    # 7. the rotation served REAL replaced content: steps now match the
    #    replacement archive (poll: the replaced model re-parses in background;
    #    the first iteration can hit that parse, so use the heavy timeout and
    #    keep retrying inside the deadline instead of letting a socket
    #    timeout fail the whole scenario)
    expected_steps = facts.steps
    deadline = time.monotonic() + 90
    steps_now = None
    while time.monotonic() < deadline:
        try:
            resp = ctx.server.http("GET", rigid_base + "/steps",
                                   timeout=HTTP_TIMEOUT_HEAVY)
        except (OSError, http.client.HTTPException):
            time.sleep(1.0)
            continue
        if resp.status == 200:
            steps_now = resp.json().get("steps")
            if steps_now == expected_steps:
                break
        time.sleep(1.0)
    sc.check_eq(steps_now, expected_steps,
                "steps list after rotation matches the REPLACED archive "
                "(expected %r, got %r)" % (expected_steps, steps_now))


def scenario_auth_disabled_accepts_unauthenticated(ctx, sc):
    """With --disable-authentication, requests WITHOUT any Authorization
    header reach the routes (the explicit 'disabled -> 200' arm of the
    auth matrix)."""
    resp = ctx.server.http("GET", "/designs")
    sc.check(resp.status in (200, 204),
             "GET /designs without auth header on disabled auth -> 2xx "
             "(got %s)" % resp.status)
    resp = ctx.server.http(
        "GET", "/filemodels/" + encode_path_segment(SAMPLE_DESIGN),
        timeout=HTTP_TIMEOUT_HEAVY)
    sc.check_eq(resp.status, 200,
                "GET /filemodels/<name> without auth header on disabled auth")


# --------------------------------------------------------------------------
# Scenarios (server A: authentication ENABLED with configured credentials)
# --------------------------------------------------------------------------

def scenario_auth_matrix(ctx, sc):
    """Real-HTTP auth matrix on GET /designs: 401 / 403 / 200 per credential
    and header shape, plus the unauthenticated health routes."""
    good = ("TestUser", "S3cretPass")
    path = "/designs"

    # no header -> 401 Unauthorized
    resp = ctx.server.http("GET", path)
    sc.check_eq(resp.status, 401, "no Authorization header -> 401")

    # malformed headers -> 401 Unauthorized (fail closed)
    for header, label in (
            ("Basic", "bare scheme token"),
            ("Basic ", "scheme with empty payload"),
            ("Bearer " + base64.b64encode(b"TestUser:S3cretPass").decode(),
             "wrong scheme"),
    ):
        resp = ctx.server.http("GET", path, headers={"Authorization": header})
        sc.check_eq(resp.status, 401, "%s -> 401" % label)

    # wrong credentials -> 403 Forbidden
    for username, password, label in (
            ("TestUser", "wrong-password", "wrong password"),
            ("wrong-user", "S3cretPass", "wrong username"),
            ("odb", "plusplus", "built-in default credential pair"),
            ("", "", "empty credentials"),
    ):
        resp = ctx.server.http("GET", path,
                               headers={"Authorization":
                                        basic_auth_header(username, password)})
        sc.check_eq(resp.status, 403, "%s -> 403" % label)

    # correct credentials -> 200 and a real listing body
    resp = ctx.server.http("GET", path, auth=good)
    if sc.check_eq(resp.status, 200, "correct credentials -> 200"):
        entries = resp.json().get("filearchives")
        listed = {e.get("name") for e in entries} if isinstance(entries,
                                                                list) else set()
        sc.check(SAMPLE_DESIGN in listed,
                 "authenticated listing includes %s" % SAMPLE_DESIGN)

    # lowercase scheme token is accepted (RFC 7235 case-insensitivity)
    payload = base64.b64encode(("%s:%s" % good).encode()).decode()
    resp = ctx.server.http("GET", path,
                           headers={"Authorization": "basic " + payload})
    sc.check_eq(resp.status, 200, "lowercase 'basic' scheme -> 200")

    # health routes are registered without the auth filter (real, documented
    # behavior): they answer 200 with NO credentials at all
    for hpath in ("/healthz/live", "/healthz", "/ready"):
        resp = ctx.server.http("GET", hpath)
        sc.check_eq(resp.status, 200,
                    "health route %s is unauthenticated (no header) -> 200"
                    % hpath)


# --------------------------------------------------------------------------
# Scenarios (server C: authentication enabled, credentials MISSING)
# --------------------------------------------------------------------------

def scenario_f004_startup_fail_closed(binary, workdir, designs_dir, sc):
    """F004 through this harness: with authentication enabled and credentials
    unconfigured, the server refuses to start (exit code 5) BEFORE any REST
    or gRPC listener opens."""
    rest_port = _free_port()
    grpc_port = _free_port()
    args = [
        binary,
        "--bind", "127.0.0.1",
        "--port", str(rest_port),
        "--grpc-port", str(grpc_port),
        "--designs-dir", _relative_designs_arg(designs_dir, workdir),
    ]
    env = _child_env(None)  # credentials scrubbed
    log_path = os.path.join(workdir, "f004_server.log")
    with open(log_path, "wb") as log_file:
        # codeql[py/command-line-injection] test harness: argv is the ctest-provided --server ($<TARGET_FILE:OdbDesignServer>), validated in main().
        process = subprocess.Popen(args, cwd=workdir, env=env, stdout=log_file, stderr=subprocess.STDOUT, start_new_session=IS_POSIX)
        try:
            process.wait(timeout=PROCESS_EXIT_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            if IS_POSIX:
                os.killpg(process.pid, 9)
            else:
                process.kill()
            process.wait(timeout=10)
            sc.check(False, "server still running after %ss (must fail "
                            "closed, not run unauthenticated)"
                     % PROCESS_EXIT_TIMEOUT_S)
            return

    sc.check(process.returncode == PRE_SERVER_RUN_FAILED_EXIT_CODE,
             "exit code is %d / PreServerRunFailed (got %r)"
             % (PRE_SERVER_RUN_FAILED_EXIT_CODE, process.returncode))
    sc.check(process.returncode > 0,
             "process exited non-zero on its own (got %r)"
             % process.returncode)

    # both ports must refuse connections AFTER the failed start
    sc.check(port_refuses(rest_port),
             "REST port %d refuses connections after fail-closed exit"
             % rest_port)
    sc.check(port_refuses(grpc_port),
             "gRPC port %d refuses connections after fail-closed exit"
             % grpc_port)


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------

def build_rootless_archive(src_archive, out_path):
    """Repackage an ODB++ archive so its members live at the tar root (no
    <design>/ prefix directory).

    WHY: the server extracts every archive for design <name> into the same
    deterministic <designs-dir>/<name>/ directory and never cleans it
    (Utils/ArchiveExtractor.cpp Extract -> parent/stem). Replacing an archive
    with DIFFERENT internal-root content therefore leaves the stale root tree
    in place next to the new one, and FileArchive::findRootDir resolves the
    root via directory_iterator order — on NTFS that order is alphabetical,
    so the STALE root can deterministically win (observed on the windows-2022
    CI leg: steps kept listing the replaced archive's design). A rootless
    layout hits findRootDir's top-level branch (fonts/misc/matrix/steps at
    the extraction root), making the replacement parse deterministic on all
    platforms. The stale-tree contamination itself is a production finding
    reported on the PR / for product triage.
    """
    prefix = None
    with tarfile.open(src_archive, "r:gz") as src:
        members = src.getmembers()
        # these fixtures are single-root archives: every member shares the
        # first member's top-level directory
        prefix = members[0].name.split("/")[0]
        with tarfile.open(out_path, "w:gz") as dst:
            for m in members:
                if m.name == prefix or m.name == prefix + "/":
                    continue  # drop the root directory member itself
                if m.name.startswith(prefix + "/"):
                    m.name = m.name[len(prefix) + 1:]
                m.mtime = int(time.time())
                fh = src.extractfile(m)
                dst.addfile(m, fh if m.isfile() and fh is not None else None)
    return out_path


def prepare_workdir(test_data_dir):
    workdir = tempfile.mkdtemp(prefix="odbdesign_rest_proc_")
    designs_dir = os.path.join(workdir, "designs")
    os.makedirs(designs_dir)
    sample_src = os.path.join(test_data_dir, SAMPLE_ARCHIVE)
    rigid_src = os.path.join(test_data_dir, RIGIDFLEX_ARCHIVE)
    # codeql[py/path-injection] test harness: fixture paths from the ctest-provided ODB_TEST_DATA_DIR, validated in main().
    for src in (sample_src, rigid_src):
        if not os.path.isfile(src):
            raise HarnessError("missing test fixture archive: %s" % src)
    return workdir, designs_dir, sample_src, rigid_src


def run(args):
    workdir, designs_dir, sample_src, rigid_src = prepare_workdir(
        args.test_data_dir)
    facts_sample = ArchiveFacts(sample_src)
    facts_rigidflex = ArchiveFacts(rigid_src)
    replacement_path = build_rootless_archive(
        sample_src, os.path.join(workdir, "rootless_sample_replacement.tgz"))
    print("workdir: %s" % workdir)
    print("sample_design: %d steps, %d layers, %d nets, %d components, "
          "%d packages, %d parts"
          % (len(facts_sample.steps),
             len(facts_sample.layers[facts_sample.steps[0]]),
             len(facts_sample.nets), sum(facts_sample.cmp_counts.values()),
             facts_sample.pkg_count, len(facts_sample.part_names)))

    results = []  # (name, ok)
    server_b = None
    server_a = None
    try:
        # ---- Server B: authentication disabled (route behavior) ----------
        server_b = start_server(args.server, designs_dir, workdir,
                                disable_auth=True, creds=None,
                                log_name="server_b.log")
        ctx = Context(server_b, designs_dir, facts_sample, facts_rigidflex,
                      replacement_path)
        for scenario_fn in (
                scenario_health_endpoints,
                scenario_design_list_empty_is_204,
                scenario_octet_stream_upload_stores_exact_bytes,
                scenario_multipart_upload_stores_exact_bytes,
                scenario_unsupported_content_type_rejected,
                scenario_duplicate_octet_upload_conflict_keeps_original,
                scenario_duplicate_multipart_upload_conflict_keeps_original,
                scenario_hostile_upload_names_rejected,
                scenario_design_list_populated,
                scenario_filemodel_route_matrix,
                scenario_product_model_routes,
                scenario_conditional_get_etag_flow,
                scenario_auth_disabled_accepts_unauthenticated,
        ):
            sc = Scenario(scenario_fn.__name__)
            try:
                scenario_fn(ctx, sc)
            except Exception as exc:  # noqa: BLE001 - report and continue
                sc.check(False, "scenario raised: %s\n%s"
                         % (exc, traceback.format_exc()))
            results.append((sc.name, sc.finalize()))

        # ---- Server A: authentication enabled, credentials configured ----
        auth_designs = os.path.join(workdir, "designs_auth")
        os.makedirs(auth_designs)
        # codeql[py/path-injection] test harness: fixture path from the ctest-provided ODB_TEST_DATA_DIR, validated in main()/prepare_workdir().
        shutil.copyfile(sample_src, os.path.join(auth_designs, SAMPLE_ARCHIVE))
        server_a = start_server(args.server, auth_designs, workdir,
                                disable_auth=False,
                                creds=("TestUser", "S3cretPass"),
                                log_name="server_a.log")
        ctx_a = Context(server_a, auth_designs, facts_sample, facts_rigidflex,
                        replacement_path)
        sc = Scenario(scenario_auth_matrix.__name__)
        try:
            scenario_auth_matrix(ctx_a, sc)
        except Exception as exc:  # noqa: BLE001
            sc.check(False, "scenario raised: %s\n%s"
                     % (exc, traceback.format_exc()))
        results.append((sc.name, sc.finalize()))

        # ---- Server C: authentication enabled, credentials missing ------
        sc = Scenario(scenario_f004_startup_fail_closed.__name__)
        try:
            scenario_f004_startup_fail_closed(args.server, workdir,
                                              designs_dir, sc)
        except Exception as exc:  # noqa: BLE001
            sc.check(False, "scenario raised: %s\n%s"
                     % (exc, traceback.format_exc()))
        results.append((sc.name, sc.finalize()))

    finally:
        for server in (server_a, server_b):
            if server is not None:
                server.stop()
        if not args.keep_logs:
            shutil.rmtree(workdir, ignore_errors=True)
        else:
            print("kept workdir: %s" % workdir)

    # ---- summary ---------------------------------------------------------
    print("\n==== RestApiProcessTests summary ====")
    failed = 0
    for name, ok in results:
        print("  [%s] %s" % ("PASS" if ok else "FAIL", name))
        if not ok:
            failed += 1
    total = len(results)
    print("scenarios: %d, passed: %d, failed: %d"
          % (total, total - failed, failed))
    return EXIT_ALL_PASSED if failed == 0 else EXIT_SCENARIOS_FAILED


def _to_number(text):
    if text is None:
        return None
    try:
        return float(text)
    except ValueError:
        return text


def main(argv):
    parser = argparse.ArgumentParser(
        description="OdbDesignServer external-process HTTP acceptance tests")
    parser.add_argument("--server", required=True,
                        help="path to the OdbDesignServer executable")
    parser.add_argument("--test-data-dir",
                        default=os.environ.get("ODB_TEST_DATA_DIR"),
                        help="directory holding sample_design.tgz and "
                             "designodb_rigidflex.tgz (defaults to "
                             "ODB_TEST_DATA_DIR)")
    parser.add_argument("--keep-logs", action="store_true",
                        help="keep the temp workdir (server logs) for "
                             "debugging")
    args = parser.parse_args(argv)

    # Popen runs the server with cwd set to the temp workdir, so a relative
    # --server path must be resolved against the harness's own cwd first.
    args.server = os.path.abspath(args.server)
    # codeql[py/path-injection] test harness: --server is the CMake ctest registration ($<TARGET_FILE:OdbDesignServer>), validated below.
    if not os.path.isfile(args.server):
        print("ERROR: server binary not found: %s" % args.server,
              file=sys.stderr)
        return EXIT_USAGE_ERROR
    # codeql[py/path-injection] test harness: --server is the CMake ctest registration ($<TARGET_FILE:OdbDesignServer>), validated here.
    if not os.access(args.server, os.X_OK):
        print("ERROR: server binary is not executable: %s" % args.server,
              file=sys.stderr)
        return EXIT_USAGE_ERROR
    if not args.test_data_dir:
        # Same posture as the gtest fixtures (GTEST_SKIP without fixtures):
        # no test data on this machine is an environment-driven SKIP, not a
        # product failure. ctest maps exit code 3 to SKIPPED via the
        # SKIP_RETURN_CODE test property.
        print("SKIPPED (RestApiProcessTests): ODB_TEST_DATA_DIR is not set "
              "and --test-data-dir was not given; design fixtures are "
              "required for the external-process HTTP suite")
        return EXIT_SKIPPED_NO_TEST_DATA
    # codeql[py/path-injection] test harness: fixture directory is the ctest-provided ODB_TEST_DATA_DIR / --test-data-dir, validated here.
    if not os.path.isdir(args.test_data_dir):
        print("ERROR: test data dir not found: %s" % args.test_data_dir,
              file=sys.stderr)
        return EXIT_USAGE_ERROR
    try:
        return run(args)
    except HarnessError as exc:
        # broken fixture checkout (missing archives): clean usage error, no
        # traceback
        print("ERROR: %s" % exc, file=sys.stderr)
        return EXIT_USAGE_ERROR


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
