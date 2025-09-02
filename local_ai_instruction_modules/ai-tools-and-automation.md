# GitHub Automation Rules & Tool Discovery Protocol

## Overview
This document establishes mandatory protocols for maximizing automation and tool usage in GitHub-based workflows, preventing manual execution when automated alternatives exist.

## Mandatory Tool Discovery Protocol

### Pre-Assignment Checkpoint
Before beginning ANY GitHub-related assignment:

1. **Inventory Available Tools** (MANDATORY)
   - Execute systematic tool discovery using available MCP tools
   - Document ALL GitHub-related tools found
   - Map tools to planned assignment tasks
   - **MINIMUM REQUIREMENT**: Achieve 90% task automation coverage

2. **Tool Capability Assessment**
   - Test tool availability and functionality
   - Verify tool permissions and scope
   - Document any tool limitations or gaps

3. **Automation Strategy Planning**
   - Prioritize automated approaches over manual execution
   - Create fallback plans only when tools are unavailable
   - Document justification for any manual steps

### Success Metrics

#### Automation Coverage Target
- **Primary Metric**: Achieve minimum 90% automation coverage for all GitHub operations
- **Measurement**: (Automated Tasks / Total Tasks) × 100 ≥ 90%
- **Tracking**: Document automation percentage for each assignment
- **Escalation**: If automation coverage falls below 90%, mandatory tool re-evaluation required

#### Tool Utilization Standards
- **Discovery Completeness**: 100% of available GitHub tools must be identified before assignment start
- **Implementation Rate**: Use automated tools for all compatible tasks (0% acceptable manual execution for automatable tasks)
- **Documentation Requirement**: Explicitly justify any manual steps with tool limitation evidence

#### Quality Assurance
- **Verification**: All automated operations must include success confirmation
- **Error Handling**: Automated failures require immediate tool troubleshooting before manual fallback
- **Continuous Improvement**: Update tool knowledge base after each assignment completion

## GitHub Automation Hierarchy

### Priority 1: MCP GitHub Tools (USE FIRST)
- `mcp_github_*` tools for all repository operations
- Issue creation, labeling, milestone management
- Repository configuration and project setup
- Sub-issue linking and epic management

### Priority 2: VS Code GitHub Integration
- Built-in GitHub commands via `run_vscode_command`
- Extension-based GitHub operations
- Integrated pull request and issue management

### Priority 3: Terminal GitHub CLI (LAST RESORT)
- `gh` commands via `run_in_terminal`
- Only when MCP tools unavailable
- Requires explicit justification

### PROHIBITED: Manual GitHub Web Interface
- Manual issue creation in browser
- Manual label/milestone configuration
- Manual repository setup steps

## Assignment Template Enhancement

### New Required Sections
All assignment templates must include:

```markdown
## Automation Checkpoint
- [ ] Tool discovery completed (100% coverage)
- [ ] Automation strategy documented
- [ ] Manual steps justified with tool limitations
- [ ] Target automation coverage: ≥90%

## Tool Usage Justification
| Task | Tool Used | Automation Status | Manual Justification |
|------|-----------|-------------------|---------------------|
| [Task] | [Tool] | [Auto/Manual] | [Reason if manual] |
```

## Behavioral Changes

### "Automate First, Manual Last" Principle
1. **Default Assumption**: Every GitHub task can be automated
2. **Tool-First Approach**: Always attempt automated solution first
3. **Manual Exception**: Manual execution requires documented tool limitation
4. **Continuous Learning**: Update tool knowledge after each workflow

### Systematic Tool Exploration
- Maintain updated inventory of all available GitHub automation tools
- Regular testing of tool capabilities and limitations
- Documentation of tool evolution and new features

### Process Validation
- Post-assignment automation coverage review
- Tool usage effectiveness analysis
- Continuous improvement of automation strategies

## Implementation Checklist

### Immediate Actions
- [ ] Create this document in repository docs folder
- [ ] Update orchestrate-dynamic-workflow.prompt.md with automation checkpoint
- [ ] Enhance assignment templates with automation requirements
- [ ] Establish tool discovery protocol in workflow guardrails

### Ongoing Requirements
- [ ] Maintain 90% automation coverage standard
- [ ] Regular tool capability updates
- [ ] Assignment-level automation reporting
- [ ] Continuous process improvement based on automation metrics

## Monitoring & Reporting

### Assignment-Level Tracking
Each assignment completion must include:
- Automation coverage percentage
- Tool usage breakdown
- Manual step justifications
- Process improvement recommendations

### Workflow-Level Analysis
Monthly review of:
- Overall automation trends
- Tool adoption rates
- Process optimization opportunities
- Success metric performance against 90% target

---

*This document serves as the authoritative guide for GitHub automation practices. Adherence to these protocols is mandatory for all GitHub-related workflows and assignments.*

## Tools Inventory (Workspace Snapshot)

Use the selected toolset file to enumerate exactly which tools are enabled in this workspace. Treat this file as the source of truth for tool availability.

- Selected tools (enabled = 126): [toolset.selected.json](./toolset.selected.json)
- Disabled tools: web-fetch, placeholder edit tool, and new-workspace scaffolding

Guidance:
- Always prefer MCP GitHub tools (`mcp_github_*`) for repo/project/issue operations.
- Use VS Code GitHub integration next (`run_vscode_command`).
- Use terminal `gh` only when neither of the above can fulfill the task; include justification per Automation Checkpoint.