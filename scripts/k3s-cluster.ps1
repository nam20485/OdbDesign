param(
    # Lifecycle action to perform on the k3s cluster
    [ValidateSet('Install','Start','Stop','Restart','Status','Uninstall')]
    [string]$Action = 'Status',
    # SANs baked into the k3s serving cert at first start (Install only)
    [string[]]$TlsSans = @('192.168.122.200','100.118.225.119'),
    # k3s version pin (exported as INSTALL_K3S_VERSION). Empty = latest stable
    [string]$K3sVersion = '',
    # Install: re-run the installer over an existing install. Uninstall: skip the confirmation
    [switch]$Force = $false,
    # How long to wait for the node to report Ready (applies to Install, Start and Restart)
    [int]$ReadyTimeoutSeconds = 300
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($IsWindows) {
    throw "k3s-cluster.ps1 must run inside the Debian VM (Linux), not on the Windows host."
}

$lanIp = '192.168.122.200'
$requiredPorts = @(80, 443, 6443, 50051)
$kubeDir = Join-Path $HOME '.kube'
$kubeConfigPath = Join-Path $kubeDir 'config'
$installerUrl = 'https://get.k3s.io'
$installerPath = '/tmp/k3s-install.sh'
$k3sBinaryPath = '/usr/local/bin/k3s'
$k3sUnitPath = '/etc/systemd/system/k3s.service'
$k3sUninstallScriptPath = '/usr/local/bin/k3s-uninstall.sh'
$minFreeKb = [int64]5 * 1024 * 1024

function Get-ListeningPorts {
    # Parse the Local Address:Port column of `ss -tln` in PowerShell (no brittle grep)
    $ports = @()
    foreach ($line in (ss -tln 2>$null)) {
        if ($line -match '^LISTEN' -and $line -match ':(\d+)\s') {
            $ports += [int]$Matches[1]
        }
    }
    return ,$ports
}

function Get-BusyRequiredPorts {
    $listening = Get-ListeningPorts
    return ,@($requiredPorts | Where-Object { $listening -contains $_ })
}

function Test-K3sInstalled {
    return (Test-Path $k3sBinaryPath) -or (Test-Path $k3sUnitPath)
}

function Get-InstalledTlsSans {
    if (-not (Test-Path $k3sUnitPath)) {
        return ,@()
    }
    $content = Get-Content $k3sUnitPath -Raw
    return ,([regex]::Matches($content, '--tls-san[=\s]+(\S+)') | ForEach-Object { $_.Groups[1].Value })
}

function Wait-NodeReady {
    param(
        [int]$TimeoutSeconds
    )

    Write-Host "Waiting up to $TimeoutSeconds s for the node to report Ready..."
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $ready = sudo k3s kubectl get nodes -o 'jsonpath={.items[0].status.conditions[?(@.type=="Ready")].status}' 2>$null
        if ($ready -eq 'True') {
            Write-Host "Node is Ready."
            return
        }
        Start-Sleep -Seconds 5
    }
    throw "Node did not report Ready within $TimeoutSeconds s. Check 'sudo journalctl -u k3s -n 100'."
}

function Backup-KubeConfig {
    if (Test-Path $kubeConfigPath) {
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
        $backupPath = "$kubeConfigPath.backup-$stamp"
        Copy-Item $kubeConfigPath $backupPath
        Write-Host "Backed up existing kubeconfig to $backupPath"
        return $backupPath
    }
    return $null
}

function Restore-KubeConfig {
    $latestBackup = Get-ChildItem $kubeDir -Filter 'config.backup-*' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($null -ne $latestBackup) {
        Copy-Item $latestBackup.FullName $kubeConfigPath -Force
        Write-Host "Restored kubeconfig from $($latestBackup.FullName)"
        return
    }
    if (Test-Path $kubeConfigPath) {
        Remove-Item $kubeConfigPath
        Write-Host "Removed kubeconfig pointing at the uninstalled cluster."
    }
}

function Assert-InstallPreFlight {
    foreach ($tool in @('curl', 'sudo')) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "Required tool '$tool' is not available."
        }
    }

    $busyPorts = Get-BusyRequiredPorts
    if ($busyPorts.Count -gt 0) {
        throw "Port(s) $($busyPorts -join ', ') already listening. Free them before installing (inspect with 'ss -tlnp')."
    }

    $freeKb = [int64](df --output=avail -k / | Select-Object -Last 1)
    if ($freeKb -lt $minFreeKb) {
        throw "Insufficient free disk space on /: $([math]::Round($freeKb / 1024 / 1024, 1)) GB free, need at least 5 GB."
    }
}

function Invoke-Install {
    Assert-InstallPreFlight
    Backup-KubeConfig | Out-Null

    if (Test-K3sInstalled) {
        if (-not $Force) {
            throw "k3s is already installed. Re-run with -Force to re-run the installer over the existing install."
        }

        Write-Host "WARNING: -Force specified, re-running the installer over the existing install."
        $existingSans = @(Get-InstalledTlsSans)
        if ($null -ne $existingSans -and $existingSans.Count -gt 0) {
            $changed = @(Compare-Object $existingSans $TlsSans -ErrorAction SilentlyContinue)
            if ($changed.Count -gt 0) {
                Write-Host "WARNING: -TlsSans ($($TlsSans -join ', ')) differ from the installed unit's ($($existingSans -join ', '))."
                Write-Host "WARNING: cert SANs are baked at first start; re-running with different --tls-san does NOT rotate the serving cert."
                Write-Host "WARNING: a cert rotation or full reinstall is required for new SANs to take effect."
            }
        }
    }

    Write-Host "Downloading k3s installer from $installerUrl ..."
    curl -sfL $installerUrl -o $installerPath

    $tlsArgs = @()
    foreach ($san in $TlsSans) {
        $tlsArgs += '--tls-san'
        $tlsArgs += $san
    }

    if ($K3sVersion) {
        Write-Host "Installing k3s $K3sVersion (tls-san: $($TlsSans -join ', '))..."
        sudo "INSTALL_K3S_VERSION=$K3sVersion" sh $installerPath @tlsArgs
    }
    else {
        Write-Host "Installing latest stable k3s (tls-san: $($TlsSans -join ', '))..."
        sudo sh $installerPath @tlsArgs
    }

    sudo systemctl enable k3s
    Wait-NodeReady -TimeoutSeconds $ReadyTimeoutSeconds

    Write-Host "Copying kubeconfig for user $env:USER..."
    New-Item -ItemType Directory -Force -Path $kubeDir | Out-Null
    sudo cp /etc/rancher/k3s/k3s.yaml $kubeConfigPath
    sudo chown "${env:USER}:${env:USER}" $kubeConfigPath
    chmod 600 $kubeConfigPath
    (Get-Content $kubeConfigPath) -replace 'https://127\.0\.0\.1:6443', "https://${lanIp}:6443" | Set-Content $kubeConfigPath

    Write-Host ""
    Write-Host "k3s installed. Access map:"
    Write-Host "  Kubernetes API : https://${lanIp}:6443 and https://100.118.225.119:6443 (tls-san)"
    Write-Host "  Ingress        : http://${lanIp}/ and http://100.118.225.119/ (k3s default Traefik, 80/443)"
    Write-Host "  gRPC           : ${lanIp}:50051 and 100.118.225.119:50051 (ServiceLB, after deploy)"
    Write-Host "  kubeconfig     : /etc/rancher/k3s/k3s.yaml (root, 600), copied to $kubeConfigPath"
    kubectl get nodes
}

function Invoke-Status {
    if (-not (Test-K3sInstalled)) {
        Write-Host "k3s is not installed (no $k3sBinaryPath)."
    }

    $activeState = (systemctl is-active k3s 2>$null) -join ''
    $enabledState = (systemctl is-enabled k3s 2>$null) -join ''
    if (-not $activeState) { $activeState = 'unknown' }
    if (-not $enabledState) { $enabledState = 'unknown' }
    Write-Host "k3s unit: active=$activeState enabled=$enabledState"

    $listening = Get-ListeningPorts
    Write-Host "Listening ports of interest:"
    foreach ($port in $requiredPorts) {
        $state = if ($listening -contains $port) { 'LISTENING' } else { 'free' }
        Write-Host "  ${port}: $state"
    }

    if ($activeState -ne 'active') {
        Write-Host "k3s is not active; skipping kubectl sections."
        return
    }

    kubectl get nodes -o wide
    kubectl get pods -A
}

function Invoke-Uninstall {
    if (-not (Test-K3sInstalled)) {
        Write-Host "k3s is not installed; nothing to uninstall."
        return
    }

    Write-Host "WARNING: k3s-uninstall.sh runs a kill-all script that can kill processes belonging to other container runtimes."
    if (-not $Force) {
        $confirmInput = Read-Host "Type 'uninstall' and hit ENTER to uninstall k3s..."
        if ($confirmInput -ne 'uninstall') {
            Write-Host "Confirmation did not match. Exiting..."
            exit 1
        }
    }
    else {
        Write-Host "Force uninstall specified..."
    }

    sudo $k3sUninstallScriptPath
    Restore-KubeConfig

    $activeState = (systemctl is-active k3s 2>$null) -join ''
    if ($activeState -eq 'active') {
        throw "k3s unit is still active after uninstall."
    }
    if (Test-Path $k3sBinaryPath) {
        throw "$k3sBinaryPath still exists after uninstall."
    }
    $busyPorts = Get-BusyRequiredPorts
    if ($busyPorts.Count -gt 0) {
        throw "Port(s) $($busyPorts -join ', ') still listening after uninstall."
    }
    Write-Host "k3s uninstalled and validated (unit inactive, binary removed, ports free)."
}

if ($Action -in @('Install', 'Start', 'Stop', 'Restart', 'Uninstall')) {
    Write-Host "Caching sudo credentials (note: the sudo timeout is short when running unattended)..."
    sudo -v
}

switch ($Action) {
    'Install' {
        Invoke-Install
    }
    'Start' {
        $activeState = (systemctl is-active k3s 2>$null) -join ''
        if ($activeState -eq 'active') {
            Write-Host "k3s is already active."
            Wait-NodeReady -TimeoutSeconds $ReadyTimeoutSeconds
            return
        }
        sudo systemctl start k3s
        Wait-NodeReady -TimeoutSeconds $ReadyTimeoutSeconds
    }
    'Stop' {
        sudo systemctl stop k3s
        Write-Host "k3s stopped."
    }
    'Restart' {
        sudo systemctl restart k3s
        Wait-NodeReady -TimeoutSeconds $ReadyTimeoutSeconds
    }
    'Status' {
        Invoke-Status
    }
    'Uninstall' {
        Invoke-Uninstall
    }
}
