Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nam20485/OdbDesign/development/.github/setup-environment.sh" `
            -OutFile ".github/setup-environment.sh" `
            -Headers @{"Accept"="application/vnd.github.raw"}
