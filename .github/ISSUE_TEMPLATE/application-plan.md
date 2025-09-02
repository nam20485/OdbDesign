Title: [ProjectName] – Complete Implementation (Application Plan)

Labels: plan, design, architecture
Assignees: [owner]

---

# [ProjectName] – Complete Implementation

## Overview
Provide a concise summary of the application, the problem it solves, the desired outcomes, and links to the filled-out template (docs/ai-new-app-template.md) and any supporting docs.

## Goals
- [Goal 1]
- [Goal 2]

## Technology Stack
- Language: [e.g., C# .NET 9.0]
- UI Framework: [e.g., Avalonia/Blazor/etc.]
- AI/Runtime: [e.g., ONNX Runtime / Azure OpenAI / etc.]
- Architecture: [e.g., RAG / MCP / Microservices]
- Databases/Storage: [e.g., Neo4j / SQLite / Postgres / Vector DB]
- Logging/Observability: [e.g., Serilog, OpenTelemetry]
- Containerization/Infra: [e.g., Docker, Compose, Terraform]

## Application Features
- [Feature 1]
- [Feature 2]
- [Feature 3]

## System Architecture
### Core Services (if applicable)
1. [ServiceName] — responsibility summary
2. [ServiceName] — responsibility summary

### Key Features (system-level)
- [Key feature/capability]

## Project Structure
```
[ProjectName]/
├─ src/
│  ├─ [Project].Core/
│  ├─ [Project].Api/
│  ├─ [Project].Frontend/
│  └─ [Project].Shared/
├─ tests/
├─ docs/
├─ scripts/
├─ docker/
├─ assets/
└─ global.json
```

---

## Implementation Plan

### Phase 1: Foundation & Setup
- [ ] 1.1. Repository and solution bootstrap
- [ ] 1.2. Core dependencies and configuration
- [ ] 1.3. Runtime/Model initialization (if AI)
- [ ] 1.4. Data/Knowledge base foundation (if RAG)
- [ ] 1.5. Basic content processing/indexing (if applicable)

### Phase 2: Core Services / Core Engine
- [ ] 2.1. Implement core module/service A
   - [ ] 2.1.1. Sub-task A
   - [ ] 2.1.2. Sub-task B
- [ ] 2.2. Implement core module/service B

### Phase 3: UI/UX & Integration
- [ ] 3.1. UI foundation and navigation
- [ ] 3.2. ViewModels/State management
- [ ] 3.3. Primary user flows (chat/task/…)
- [ ] 3.4. Async ops, cancellation, error handling
- [ ] 3.5. Settings/configuration

### Phase 4: Advanced Capabilities & Security
- [ ] 4.1. Tooling/Function calling/Agentic features (if applicable)
- [ ] 4.2. Human-in-the-loop approval and auditing
- [ ] 4.3. Performance optimizations and caching
- [ ] 4.4. Observability and dashboards

### Phase 5: Testing, Docs, Packaging & Deployment
- [ ] 5.1. Test suites (unit/integration/e2e/perf)
- [ ] 5.2. API/Developer documentation
- [ ] 5.3. Containerization/installer packaging
- [ ] 5.4. IaC/Environments/CI-CD pipelines
- [ ] 5.5. Final hardening and release checklist

---

## Mandatory Requirements Implementation

### Testing & Quality Assurance
- [ ] Unit tests — coverage target: [e.g., 80%+]
- [ ] Integration tests
- [ ] E2E tests
- [ ] Performance/load tests
- [ ] Automated tests in CI

### Documentation & UX
- [ ] Comprehensive README
- [ ] User manual and feature docs
- [ ] XML/API docs (public APIs)
- [ ] Troubleshooting/FAQ
- [ ] In-app help (if applicable)

### Build & Distribution
- [ ] Build scripts
- [ ] Containerization support (if relevant)
- [ ] Installer packaging (if desktop)
- [ ] Release pipeline

### Infrastructure & DevOps
- [ ] CI/CD workflows (build/test/scan/publish)
- [ ] Static analysis and security scanning
- [ ] Performance benchmarking/monitoring

---

## Acceptance Criteria
- [ ] Core architecture implemented and components communicate as designed
- [ ] Key features/functionality work end-to-end
- [ ] Observability/logging in place with actionable signals
- [ ] Security model and controls validated
- [ ] Test coverage target met and CI green
- [ ] Containerization/packaging works for target environment(s)
- [ ] Documentation complete and accurate

## Risk Mitigation Strategies
| Risk | Mitigation |
|------|------------|
| [Risk 1] | [Mitigation 1] |
| [Risk 2] | [Mitigation 2] |

## Timeline Estimate
- Phase 1: [x–y] weeks
- Phase 2: [x–y] weeks
- Phase 3: [x–y] weeks
- Phase 4: [x–y] weeks
- Phase 5: [x–y] weeks
- Total: [x–y] weeks

## Success Metrics
- [Metric 1]
- [Metric 2]
- [Metric 3]

## Repository Branch
Target branch for implementation: [e.g., copilot/[project-slug]]

## Implementation Notes
Key assumptions, adaptations, and references to technical docs or ADRs.