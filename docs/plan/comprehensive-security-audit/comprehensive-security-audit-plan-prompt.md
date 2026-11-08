# comprehensive-security-audit-plan

Use a powerful model to conduct a comprehensive secutiry audit over the entire repository.

## Structure

Us epowerful model to orchestrate multiple parallel subagents.

- subagents each conduct independent securtity audit over whoe repo
- upon completion, orchestrator collates individual subagent findings, validates all findings, and combines into a single coherent secruty audit report.
- all subagents findings must be validated independently by a seperate, object agent. Take nothing at face, all findings must be validated and provide source code references to support claims.

### Models

- `orchestrator:` `Kimi K3.1 Pro`
- `subagents`:
   - kimi code k2.8  
   - qwen 3.8 flash (max effort)
   - glm 5.3-flash (max effort)
- independent validators: `glm 5.3 (high effort)

## Output

Document results in a markdown document.

- `docs/plan/comprehensive-security-audit/comprehensive-security-audit.md`
