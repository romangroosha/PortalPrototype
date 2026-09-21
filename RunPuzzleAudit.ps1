$projectFile = Join-Path $PSScriptRoot 'PortalPrototype.uproject'
$auditLog = Join-Path $PSScriptRoot 'Saved\PuzzleAudit.log'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectFile '/Game/Maps/PortalPuzzle' '-game' '-unattended' '-RenderOffscreen' '-windowed' '-ForceRes' '-ResX=1280' '-ResY=720' '-nosplash' '-ddc=InstalledNoZenLocalFallback' '-PortalPuzzleAudit' '-ExecCmds=t.IdleWhenNotForeground 0,r.VSync 0,t.MaxFPS 60,r.ScreenPercentage 100' "-abslog=$auditLog"
if ($LASTEXITCODE -ne 0) { throw "Puzzle audit failed: $LASTEXITCODE" }
if (-not (Select-String -LiteralPath $auditLog -Pattern 'PORTAL_PUZZLE_AUDIT PASS' -Quiet)) { throw 'Puzzle audit did not report PASS.' }
& python (Join-Path $PSScriptRoot 'Scripts\check_puzzle_images.py')
if ($LASTEXITCODE -ne 0) { throw 'Portal image regression check failed.' }
Select-String -LiteralPath $auditLog -Pattern 'PUZZLE_CHECK|PORTAL_PUZZLE_AUDIT'
