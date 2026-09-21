param([string]$Message)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
function Invoke-Git {
    & git @args
    if ($LASTEXITCODE -ne 0) { throw "Git failed (exit code $LASTEXITCODE)." }
}
try {
    Write-Host 'Save All in Unreal Editor before saving a version.'
    if (-not $Message) { $Message = Read-Host 'Version description (Enter = timestamp)' }
    if (-not $Message) { $Message = 'Snapshot ' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') }
    Invoke-Git add --all
    & git diff --cached --quiet
    $diffExit = $LASTEXITCODE
    if ($diffExit -eq 1) { Invoke-Git commit -m $Message }
    elseif ($diffExit -eq 0) { Write-Host 'No changes to save.' }
    else { throw 'Cannot inspect staged changes.' }
    $remotes = @(Invoke-Git remote)
    if ($remotes -contains 'origin') {
        Invoke-Git push origin HEAD
        Write-Host 'Version saved locally and uploaded to origin.'
    } else {
        Write-Host 'Version saved locally. No remote backup configured yet.'
    }
} catch {
    Write-Host $_ -ForegroundColor Red
    exit 1
}
