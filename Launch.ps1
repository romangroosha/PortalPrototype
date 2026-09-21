$projectFile = Join-Path $PSScriptRoot 'PortalPrototype.uproject'
& 'C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' $projectFile '-ddc=InstalledNoZenLocalFallback'
