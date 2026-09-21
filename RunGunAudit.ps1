$projectFile = Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog = Join-Path $PSScriptRoot 'Saved\PortalGunAudit.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalGunLab' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1920' '-ResY=1080' '-nosplash' '-ddc=InstalledNoZenLocalFallback' '-PortalGunAudit' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 60,r.ScreenPercentage 100' "-abslog=$auditLog"
if ($LASTEXITCODE -ne 0) { throw "Portal gun audit failed: $LASTEXITCODE" }
if (-not (Select-String -LiteralPath $auditLog -Pattern 'PORTAL_GUN_AUDIT PASS' -Quiet)) { throw 'Audit did not report PASS.' }
Select-String -LiteralPath $auditLog -Pattern 'PORTAL_GUN_AUDIT PASS'
