---
mode: agent
---

/orchestrate-dynamic-workflow
  - $workflow_name = `project-setup`,
  - $context = {
      repo_name = "advanced_memory",
      app_plan_docs = [ #file:Advanced Memory .NET - Dev Plan.md , #file:index.html  ]
    }