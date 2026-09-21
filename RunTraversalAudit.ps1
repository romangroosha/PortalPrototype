$projectFile=Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog=Join-Path $PSScriptRoot 'Saved\TraversalAudit.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalFoundationLab' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1280' '-ResY=720' '-ddc=InstalledNoZenLocalFallback' '-PortalTraversalAudit' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 60' "-abslog=$auditLog"
if($LASTEXITCODE -ne 0 -or -not(Select-String -LiteralPath $auditLog -Pattern 'PORTAL_TRAVERSAL_AUDIT PASS' -Quiet)){throw 'Traversal audit failed'}
Select-String -LiteralPath $auditLog -Pattern 'TRAVERSAL_CASE|TRAVERSAL_CHECK|PORTAL_TRAVERSAL_AUDIT'
