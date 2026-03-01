>Title: [ProjectName] – [FeatureName] Feature Plan

>Labels: plan, feature, documentation

>Assignees: [owner]

---

# [ProjectName] – [FeatureName] Feature Plan

## Overview
Summarize the feature objective, the user/business problem it solves, expected outcomes, and link to the feature brief plus supporting documents located under `plan_docs/` (e.g., `feature-brief.md`, `tech-stack.md`, `architecture.md`, ADRs, UX flows).

## Current State Assessment
- Existing functionality summary
- Known limitations or pain points motivating the feature
- Key services/APIs/modules involved today
- Relevant usage metrics or telemetry

## Feature Goals & Objectives
- [Goal 1]
- [Goal 2]
- [Goal 3]

## Scope Definition
### In Scope
- [Item]
- [Item]

### Out of Scope / Deferred
- [Item]
- [Item]

## Dependencies & Integration Points
- Internal components impacted (UI, API, services, background jobs, data pipelines)
- External systems or third-party integrations
- Shared libraries or cross-team ownership considerations
- Feature flags, configuration, or runtime toggles required

## Architecture & Design Deltas
### High-Level Changes
- Summary of how the feature modifies existing architecture
- New components/services required (if any)

### Component & Service Impact
| Component | Current Responsibility | Planned Change | Notes |
|-----------|------------------------|----------------|-------|
| [Module]  | [Description]          | [Change]       | [Notes] |

### Data Model & Storage Updates
- Schema changes, migrations, indexing needs
- Data lifecycle considerations (retention, privacy, compliance)

### Communication & Contracts
- API additions or updates
- Messaging/event contract changes
- Backward compatibility strategy

## Project Structure Impact (Planned)
```
[ProjectName]/
├─ src/
│  ├─ …existing code…
│  ├─ [Feature].Domain/
│  ├─ [Feature].Application/
│  ├─ [Feature].Api/
│  └─ [Feature].Ui/
├─ tests/
│  ├─ [Feature].Domain.Tests/
│  └─ [Feature].E2E/
├─ docs/
│  ├─ plan_docs/
│  │  ├─ feature-brief.md
│  │  ├─ tech-stack.md
│  │  └─ architecture.md
└─ …existing structure…
```
_Document intended changes only; do not create directories/files during planning._

## Implementation Plan

### Phase 0: Discovery & Alignment
- [ ] Review feature brief, ADRs, UX flows, and stakeholder inputs
- [ ] Validate assumptions with product, design, QA, ops, and security
- [ ] Confirm success metrics and rollout guardrails

### Phase 1: Foundation & Enablers
- [ ] Identify prerequisite refactors or debt remediation
- [ ] Upgrade tooling, libraries, or runtime dependencies if needed
- [ ] Establish feature flags/configuration scaffolding

### Phase 2: Core Feature Development
- [ ] Backend/API changes
- [ ] Domain/business logic updates
- [ ] Data model adjustments & migrations
- [ ] Integrations with existing services or third parties

### Phase 3: Experience & Integration
- [ ] UI/UX updates (screens, components, accessibility)
- [ ] API contract documentation, SDK/CLI updates
- [ ] Telemetry, logging, and alerting enhancements
- [ ] Security, privacy, compliance validation

### Phase 4: Validation & Hardening
- [ ] Unit, integration, E2E, performance tests
- [ ] Regression suite updates (automation + manual)
- [ ] Load, resilience, and chaos scenarios (if applicable)
- [ ] Documentation updates (README, user guides, API docs)

### Phase 5: Rollout & Monitoring
- [ ] Feature flag strategy (enable/disable plans, kill switch)
- [ ] Rollout checklist and communication plan (beta → GA)
- [ ] Production monitoring dashboards, alerts, SLO/SLA updates
- [ ] Post-launch validation, support hand-off, and retrospective checkpoints

## Testing & Quality Strategy
- Unit test coverage target: [e.g., 80%+ for feature components]
- Integration/E2E scenarios and regression scope
- Performance/load/resilience testing plans
- Security, privacy, compliance validation tasks
- Automation updates required in CI/CD pipelines

## Documentation & Communication
- README/user doc updates required
- Internal enablement/training artifacts (support, CX, sales)
- API/reference documentation additions
- Stakeholder communication cadence and approval checkpoints

## Operational Readiness
- Monitoring/alerting updates and dashboards
- SLO/SLA adjustments or additions
- Runbooks, troubleshooting guides, incident response updates
- On-call/operations readiness review

## Risk Management
| Risk | Impact | Likelihood | Mitigation | Owner |
|------|--------|------------|------------|-------|
| [Risk] | [High/Med/Low] | [High/Med/Low] | [Plan] | [Name] |

## Regression Impact Log
- [Area] — [Impact] — [Mitigation/Test]
- [Area] — [Impact] — [Mitigation/Test]

## Rollout Plan
- Launch phases (beta, canary, general availability)
- Environment sequencing (dev → staging → prod)
- Rollback strategy and automation
- Post-launch monitoring and review checkpoints

## Success Metrics & KPIs
- [Metric 1]
- [Metric 2]
- [Metric 3]

## Supporting Artifacts
- `plan_docs/feature-brief.md`
- `plan_docs/tech-stack.md`
- `plan_docs/architecture.md`
- Additional diagrams / ADRs / specs: [links]

## Repository Branch & Milestones
- Target implementation branch: [e.g., feature/[slug]]
- Milestone(s): [Milestone 1], [Milestone 2]
- Linked GitHub Project(s): [Project]

## Acceptance Checklist
- [ ] Feature requirements and constraints documented
- [ ] Architecture/design deltas captured in plan_docs
- [ ] Implementation phases defined with validation checkpoints
- [ ] Testing strategy (including regression coverage) documented
- [ ] Documentation, rollout, and observability plans captured
- [ ] Risks/mitigations and regression log completed
- [ ] Milestones, labels, and project links configured
- [ ] Stakeholder approval confirmed

## Implementation Notes
Key assumptions, trade-offs, open questions, and references to decision logs or ADRs. Capture follow-up actions and ownership for outstanding items.
