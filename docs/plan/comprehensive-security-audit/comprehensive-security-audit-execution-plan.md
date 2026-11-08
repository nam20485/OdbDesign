# Comprehensive Security Audit Execution Plan

## Status

**Proposed for review — audit execution has not started.**

This plan implements the requested methodology in
[`comprehensive-security-audit-plan-prompt.md`](comprehensive-security-audit-plan-prompt.md).
It is intentionally audit-only: the purpose of this document is to establish the
review, validation, and reporting process before any audit run begins.

## 1. Objective and Scope

Conduct a comprehensive, evidence-backed security audit of the entire OdbDesign
repository at a recorded branch and commit.

The eventual audit deliverable will be:

- `docs/plan/comprehensive-security-audit/comprehensive-security-audit.md`

The audit will not include vulnerability remediation, deployment changes,
live-service probing, commits, or pushes without separate approval. Validation
may include:

- Source and configuration tracing against the audited revision.
- Bounded local checks in isolated scratch directories.
- Read-only inspection of existing GitHub workflow and security results when
  authorized.

Existing plans, prior reviews, and scanner output are leads to investigate, not
findings to accept without current evidence.

## 2. Model and Role Mapping

The mapping favors a strong orchestrator and independent validation while
avoiding maximum-effort pricing on every repository-wide review and finding.

| Role | Model and effort | Rationale |
|---|---|---|
| Orchestrator | Kimi K3.1 Pro | Synthesis, deduplication, severity arbitration, and final reporting benefit most from the strongest model. |
| Independent reviewer 1 | Kimi K3, high effort | Deep review without maximum-reasoning cost across the whole repository. This is the approved substitute for the unavailable Kimi Code K2.8 requested in the original prompt. |
| Independent reviewer 2 | Qwen 3.8 Flash, `xhigh` | Independent model family and broad coverage at lower cost. `xhigh` is its maximum available reasoning level. |
| Independent reviewer 3 | GLM 5.3 Flash, maximum effort | Independent whole-repository review and model diversity at lower cost. |
| Independent validators | GLM 5.3, high effort | Strong independent evidence checking without defaulting to maximum effort. |

Use a dynamic workflow for exact per-agent model routing. The orchestrator
remains responsible for the final collation and report rather than delegating
its acceptance decision to the reviewers.

### Cost Controls

1. Preserve three independent whole-repository reviews; do not partition the
   repository into mutually exclusive reviewer scopes.
2. Have reviewers prioritize source analysis and candidate generation instead
   of repeating identical builds, test suites, and scans.
3. Deduplicate only exact duplicate candidates before paid validation, while
   retaining every reviewer's attribution. Different attack vectors, impacts,
   preconditions, or affected configurations remain separate findings.
4. Give each unique finding one independent validator pass and run a dynamic
   check once when it is decisive.
5. Use the K3.1 Pro orchestrator to arbitrate disputed or high-impact findings
   instead of automatically adding more validator agents.
6. Do not use K3.1 Pro for every reviewer or validator, and do not use maximum
   effort everywhere. The expected quality gain does not justify the cost.

These controls reduce duplicate work without imposing a cap on legitimate
security findings.

## 3. Audit Workflow

### 3.1 Preflight and Baseline

1. Resolve the required canonical instructions and record the resolution trace,
   including URLs and available revision identifiers.
2. Record the audited branch, commit SHA, date, working-tree state, model
   roster, and approved K2.8 to K3 substitution.
3. Stop and report any unresolved required instruction or model-routing input;
   do not silently substitute another process or model.
4. Establish a common candidate-finding schema:
   - Title and affected component.
   - Current source `path:line` references.
   - Reachable data or control flow.
   - Preconditions and affected configurations.
   - Impact and proposed severity.
   - Source or dynamic validation evidence.
   - Focused remediation recommendation.
5. Use Critical, High, Medium, Low, and Informational severity based on
   demonstrated impact, exploitability, exposure, and mitigations. Do not
   present speculation as a confirmed vulnerability.

### 3.2 Three Independent Parallel Reviews

Each reviewer independently audits the whole repository without pre-seeded
conclusions. Coverage includes:

- REST and gRPC authentication, authorization, TLS assumptions, CORS,
  reflection, route coverage, and error handling.
- Upload and multipart handling, filename and path containment, temporary
  files, archive extraction, nested decompression, and subprocess invocation.
- Native ODB++ parsers, protobuf and JSON deserialization, memory safety,
  malformed record ordering, attacker-controlled indices, and fuzzing
  opportunities.
- Resource exhaustion across parsing, caching, streaming, spooling, logging,
  queues, threads, disk, RAM, and concurrent load or invalidation behavior.
- Information disclosure through responses, exceptions, logs, filesystem paths,
  diagnostics, and secret handling.
- CMake and vcpkg dependencies, build hardening, sanitizer coverage, tests,
  GitHub Actions, release provenance, artifact and image signing, SBOMs, and
  scanner configuration.
- Dockerfiles, Compose, Kubernetes, ingress and TLS, service exposure, workload
  security contexts, deployment scripts, disabled workflows, dormant surfaces,
  and security-control documentation.

Clearly distinguish active code from disabled, generated, optional, historical,
or deployment-specific surfaces. Require current source references and separate
source-only claims from dynamically demonstrated behavior.

### 3.3 Bounded Evidence Checks

1. Reuse existing fixtures and test patterns in `OdbDesignTests/`, including
   isolated archive fixtures, loopback gRPC tests, and cache/concurrency tests.
2. Use the `linux-asan` preset in `CMakePresets.json` for targeted memory-safety
   validation when a suitable check exists.
3. Run only bounded local checks with timeouts and isolated scratch
   directories. Do not deliberately exhaust host resources, access unrelated
   files, execute payloads against live services, or change cluster state.
4. Query existing GitHub workflow and security results read-only when
   authorized. Do not trigger workflows or publish repository content to an
   external service.
5. Record exact commands, exit status, relevant output, and limitations. If a
   safe check is unavailable, identify the claim as source-only or dynamically
   unverified rather than implying that a test ran.

### 3.4 Independent Validation

1. Send every unique candidate finding to a fresh GLM 5.3 high-effort
   validator. The original reviewer cannot validate its own finding.
2. Require one verdict: **Confirmed**, **Rejected**, or **Inconclusive**.
3. Support each verdict with independently checked source references and any
   decisive local evidence. Check reachability, mitigations, affected
   configurations, severity, and whether the claimed impact follows.
4. Preserve reviewer-to-finding attribution for deduplicated groups.
5. Retain rejected and inconclusive dispositions with reasons; do not silently
   discard candidates.
6. Have the orchestrator review validator results, arbitrate inconsistencies,
   and prevent unsupported claims from entering the confirmed findings.

### 3.5 Final Report Synthesis

The final `comprehensive-security-audit.md` will contain:

1. Executive summary and prioritized risk themes.
2. Audited revision, scope, methodology, model roster, approved substitution,
   and cost controls.
3. Severity and validation-status summary.
4. Confirmed findings ordered by severity, each with evidence, impact,
   preconditions, affected configurations, source references, and focused
   remediation.
5. Existing positive security controls and coverage observations, without
   treating their presence as proof of effectiveness.
6. Validation performed, tools and commands used, and read-only external
   results consulted.
7. Scope limitations and unverified areas, including inaccessible scanner data
   or unchecked runtime behavior.
8. A concise rejected and inconclusive candidate disposition appendix.
9. Reconciliation counts connecting reviewer candidates, deduplicated findings,
   validator verdicts, and final report entries.

Do not include secret values, access tokens, or unnecessary exploit detail.

## 4. Representative Audit Entry Points

The following are review targets, not planned modification targets:

- `OdbDesignServer/Controllers/FileUploadController.cpp`
- `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`
- `OdbDesignLib/App/BasicRequestAuthentication.cpp`
- `OdbDesignLib/App/DesignCache.cpp`
- `Utils/ArchiveExtractor.cpp`
- `Utils/libarchive_extract.cpp`
- `OdbDesignTests/`
- `CMakePresets.json`
- `.github/workflows/`
- `deploy/`
- `Dockerfile*`
- `compose*.yml`
- `vcpkg*`

These entry points supplement, rather than restrict, the whole-repository
review requirement.

## 5. Audit Acceptance Criteria

The later audit is complete only when:

- All three independent whole-repository reviews have completed.
- Every submitted candidate maps to a validated unique finding or an explicitly
  documented disposition.
- Every confirmed finding has current source evidence and independent
  validation; dynamic claims identify the actual checks and results.
- Rejected or inconclusive candidates are not presented as vulnerabilities.
- Severity, scope, mitigations, and limitations are consistent and supported by
  evidence.
- The report contains no secret values.
- The only intended audit deliverable change is
  `comprehensive-security-audit.md`; remediation requires separate approval.
- Any incomplete reviewer, unavailable model, failed validation gate, or
  inaccessible evidence is reported explicitly rather than described as a fully
  verified audit.

## 6. Review Gate

Present this execution plan for free-form feedback before launching the audit.
Do not start reviewer agents, scanners, builds, or report generation until the
user approves execution.
