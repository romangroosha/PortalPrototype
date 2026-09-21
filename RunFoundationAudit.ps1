$projectFile=Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog=Join-Path $PSScriptRoot 'Saved\FoundationAudit.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalFoundationLab' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1280' '-ResY=720' '-ddc=InstalledNoZenLocalFallback' '-PortalFoundationAudit' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 60' "-abslog=$auditLog"
if($LASTEXITCODE -ne 0 -or -not(Select-String -LiteralPath $auditLog -Pattern 'PORTAL_FOUNDATION_AUDIT PASS' -Quiet)){throw 'Foundation audit failed'}
Select-String -LiteralPath $auditLog -Pattern 'FOUNDATION_CHECK|PORTAL_FOUNDATION_AUDIT'
