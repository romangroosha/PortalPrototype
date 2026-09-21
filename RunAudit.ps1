$projectFile = Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog = Join-Path $PSScriptRoot 'Saved\PortalAudit.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalLab' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1920' '-ResY=1080' '-nosplash' '-ddc=InstalledNoZenLocalFallback' '-PortalAudit' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 0,r.ScreenPercentage 100,r.DynamicRes.OperationMode 0' "-abslog=$auditLog"
if ($LASTEXITCODE -ne 0) { throw "Unreal audit failed: $LASTEXITCODE" }
if (-not (Select-String -LiteralPath $auditLog -Pattern 'PORTAL_AUDIT_COMPLETE' -Quiet)) { throw 'Audit did not complete.' }
& python (Join-Path $PSScriptRoot 'Scripts\analyze_audit.py')
if ($LASTEXITCODE -ne 0) { throw 'Image analysis failed.' }
& python (Join-Path $PSScriptRoot 'Scripts\write_audit_report.py')
