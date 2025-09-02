# Instructions (Gemini)

## Where to Find Your Instructions
- Your custom instructions are located in the files inside of the [nam20485/agent-instructions](https://github.com/nam20485/agent-instructions/tree/main) repository
- Look at the files in the `main` branch
- Start with your core instructions (linked below)
- Then follow the links to the other instruction files in that repo as required or needed.
- You will need to follow the links and read the files to understand your instructions

## How to Read Your Instructions
- Read the core instructions first
- Then follow the links from the core instructions to the other instruction files
- Some files are **REQUIRED** and some are **OPTIONAL**
- Files marked **REQUIRED** are ALWAYS active and so must be followed and read
- Otherwise files are optionally active based on user needs and your assigned roles and workflow assignments

## **IMPORTANT RULES**
- Don't assume your shell is bash. Its probably pwsh. 
- Detect what type of shell you have before running any commands.
- Your web-fetch tool is disabled. Use powershell or curl to fetch files from the web.
- If there are many files, then create a pwsh script to download them in parallel.

## Core Instructions (**REQUIRED**)
[ai-core-instructions.md](https://github.com/nam20485/agent-instructions/blob/main/ai_instruction_modules/ai-core-instructions.md)

## Local AI Instructions (**REQUIRED**)
Local AI instruction moduule files are located in the [local_ai_instruction_modules](../local_ai_instruction_modules) directory.

## Dynamic Workflow Orchestration (**REQUIRED**)
Agents MUST resolve dynamic workflows from the remote canonical repository. Do not use local mirrors.
[ai-dynamic-workflows.md](../local_ai_instruction_modules/ai-dynamic-workflows.md)

## Workflow Assignments (**REQUIRED**)
Agents MUST resolve workflow assignments (by shortId) from the remote canonical repository. Do not use local mirrors.
[ai-workflow-assignments.md](../local_ai_instruction_modules/ai-workflow-assignments.md)

## Terminal Commands (Optional)
Read before running any terminal commands, of if you need Github CL.I
- [ai-terminal-commands.md](../local_ai_instruction_modules/ai-terminal-commands.md)

## Tools & Automation Protocol (REQUIRED)
Agents must prefer automation-first approaches and use only the currently-selected tools. Review the local module for the full policy and the enumerated toolset:
- [ai-tools-and-automation.md](../local_ai_instruction_modules/ai-tools-and-automation.md)
- Selected toolset JSON (exactly 126 enabled tools): [toolset.selected.json](../local_ai_instruction_modules/toolset.selected.json)

Key rules:
- Use MCP GitHub tools first, then VS Code integration, and only fall back to terminal gh as a last resort.
- The web-fetch tool is disabled; fetch remote files via PowerShell (Invoke-WebRequest) or curl.
- Maintain ≥90% automation coverage for GitHub operations and document any manual exceptions.
- If GitHub tools (MCP or gh) are not authorized, initiate the authentication/login flow (e.g., start gh auth login); the user will complete the interactive prompts.

## **Remote Repository with Main/Canonical AI Instruction Modules**

[nam20485/agent-instructions Repository](https://github.com/nam20485/agent-instructions/main)

The main set of AI instruction modules is located in this remote repository. It contains the following:
- Dynamic workflows
- Workflow assignments
- Main AI instruction modules

### Remote Repo Details

 Repository: nam20485/agent-instructions
- Full repo URL: https://github.com/nam20485/agent-instructions
- Branch: main
- Assignments directory: ai_instruction_modules/ai-workflow-assignments/
 - Active assignments index in this workspace: see `local_ai_instruction_modules/ai-workflow-assignments.md`

#### OVERRIDE NOTE
**IMPORTANT**: When accessing files in the remote repository, always use the RAW URL. Do not use the GitHub UI to view the file. The RAW URL is the URL that you get when you click on the "Raw" button in the GitHub UI. Most URLs referenced in these files of the GIT UI form. They must be translated to the RAW URL form before use. 

Examples: 

- GitHub UI for the `ai-core-instructions.md` file is: https://github.com/nam20485/agent-instructions/blob/main/ai_instruction_modules/ai-core-instructions.md

- RAW URL for the `ai-core-instructions.md` file is: https://raw.githubusercontent.com/nam20485/agent-instructions/main/ai_instruction_modules/ai-core-instructions.md.

##### Translating URLs
To translate a GitHub UI URL to a RAW URL, replace `https://github.com/` with `https://raw.githubusercontent.com/`.

*https://github.com/nam20485/agent-instructions/blob/main/<file-path> --> https://raw.githubusercontent.com/nam20485/agent-instructions/main/<file-path>*

For example, the followingGitHub UI URL: https://github.com/nam20485/agent-instructions/blob/main/ai_instruction_modules/ai-core-instructions.md 

is translated to the following RAW URL: https://raw.githubusercontent.com/nam20485/agent-instructions/main/ai_instruction_modules/ai-core-instructions.md

Examples:

- GitHub UI: https://github.com/nam20485/agent-instructions/blob/main/ai_instruction_modules/ai-core-instructions.md
- Raw URL:   https://raw.githubusercontent.com/nam20485/agent-instructions/main/ai_instruction_modules/ai-core-instructions.md

- GitHub UI: https://github.com/nam20485/agent-instructions/blob/main/ai_instruction_modules/ai-workflow-assignments/initiate-new-repository.md
- Raw URL:   https://raw.githubusercontent.com/nam20485/agent-instructions/main/ai_instruction_modules/ai-workflow-assignments/initiate-new-repository.md

Single Source of Truth Policy:

- Dynamic workflow files (under `ai_instruction_modules/ai-workflow-assignments/dynamic-workflows/`) and workflow assignment files (under `ai_instruction_modules/ai-workflow-assignments/`) in the `nam20485/agent-instructions` repository are the ONLY authoritative sources for steps and acceptance criteria.
- Local golden files, cached plans, or mirrors must not be used to derive steps or acceptance criteria. Delete any such artifacts if present.
- Changes to dynamic workflow or assignment files in the remote canonical repository take effect immediately on subsequent runs.
- The orchestrator must always fetch and execute directly from the remote canonical URLs listed below.
