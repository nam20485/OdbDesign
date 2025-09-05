# Terminal Commands Cheat Sheet (pwsh-first)

- Default shell: PowerShell (pwsh). Detect before running commands.
- Web fetch: Use PowerShell Invoke-WebRequest (or curl) — web-fetch tool is disabled.
- Prefer automation via MCP tools; terminal is last resort.

## PowerShell basics
- Current shell: `$PSVersionTable.PSEdition`, `$PSVersionTable.PSVersion`
- Download raw file: `Invoke-WebRequest -Uri <raw-url> -OutFile <path>`
- Parallel downloads: `ForEach-Object -Parallel` (PowerShell 7+)
- JSON: `Get-Content -Raw file.json | ConvertFrom-Json`

## GitHub CLI (gh)
- Auth: `gh auth status`
- Repo: `gh repo create owner/name --public|--private --template nam20485/ai-new-app-template`
- View: `gh repo view owner/name -w`
- Issues: `gh issue create -t "Title" -b "Body"`

## Git
- Clone: `git clone https://github.com/owner/name.git`
- Branch: `git checkout -B main`
- Commit: `git add -A; git commit -m "msg"; git push -u origin main`

## Safety
- Use `-WhatIf`/`-Confirm` for destructive cmdlets.
- Quote paths with `-LiteralPath` and use `Join-Path`.

---
This file exists to satisfy local docs linking and to standardize terminal usage when automation tools are unavailable.
