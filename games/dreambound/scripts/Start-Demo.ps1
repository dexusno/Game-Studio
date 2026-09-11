$ErrorActionPreference='Stop'
$reverieGameRoot=Split-Path -Parent $PSScriptRoot
$reverieExecutable=Join-Path $reverieGameRoot 'BuildOutput/CreatureAnatomy2/Windows/Dreambound.exe'
if(-not(Test-Path -LiteralPath $reverieExecutable -PathType Leaf)){
 throw 'The CreatureAnatomy2 package is missing. Build it with scripts/Build.ps1 -Stage All.'
}
# Interactive player launch, using the existing DreamboundSegments profile.
Start-Process -FilePath $reverieExecutable -WorkingDirectory (Split-Path -Parent $reverieExecutable) -WindowStyle Normal
