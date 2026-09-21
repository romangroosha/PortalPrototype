$projectFile=Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog=Join-Path $PSScriptRoot 'Saved\TraversalPreview.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalTraversalLab' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1280' '-ResY=720' '-ddc=InstalledNoZenLocalFallback' '-PortalTraversalPreview' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 60' "-abslog=$auditLog"
if($LASTEXITCODE -ne 0 -or -not(Select-String -LiteralPath $auditLog -Pattern 'TRAVERSAL_CHECK PASS: Distant floor portal' -Quiet)){throw 'Traversal preview failed'}
Select-String -LiteralPath $auditLog -Pattern 'TRAVERSAL_CHECK'
