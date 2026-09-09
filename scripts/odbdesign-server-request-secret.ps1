# Creates/updates the OdbDesignServer basic-auth secret from environment variables.
# Requires ODBDESIGN_SERVER_REQUEST_USERNAME and ODBDESIGN_SERVER_REQUEST_PASSWORD.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$secretName = "odbdesign-server-request-secret"
$username = $env:ODBDESIGN_SERVER_REQUEST_USERNAME
$password = $env:ODBDESIGN_SERVER_REQUEST_PASSWORD

if ([string]::IsNullOrWhiteSpace($username) -or [string]::IsNullOrWhiteSpace($password)) {
    $existing = kubectl get secret $secretName --ignore-not-found -o name 2>$null
    if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace("$existing")) {
        Write-Host "ODBDESIGN_SERVER_REQUEST_USERNAME/PASSWORD not set; keeping existing secret '$secretName'."
        exit 0
    }

    throw "ODBDESIGN_SERVER_REQUEST_USERNAME and ODBDESIGN_SERVER_REQUEST_PASSWORD must be set (and no existing '$secretName' secret was found)."
}

kubectl create secret generic $secretName `
    --from-literal=ODBDESIGN_SERVER_REQUEST_USERNAME=$username `
    --from-literal=ODBDESIGN_SERVER_REQUEST_PASSWORD=$password `
    --dry-run=client -o yaml | kubectl apply -f -
if ($LASTEXITCODE -ne 0) {
    throw "Failed to create/update secret '$secretName'."
}

Write-Host "Secret '$secretName' created/updated."
