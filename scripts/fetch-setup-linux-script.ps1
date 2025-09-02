

Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nam20485/agent-instructions/main/scripts/setup-environment.sh" `
            -OutFile "scripts/setup-environment.sh" `
            -Headers @{"Accept"="application/vnd.github.raw"}