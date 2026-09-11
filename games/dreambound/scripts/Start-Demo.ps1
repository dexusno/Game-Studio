$ErrorActionPreference='Stop'
$reverieGameRoot=Split-Path -Parent $PSScriptRoot
$reverieExecutable=Join-Path $reverieGameRoot 'BuildOutput/OrganicFireCandidate/Windows/Dreambound.exe'
if(-not(Test-Path -LiteralPath $reverieExecutable -PathType Leaf)){
 throw 'The OrganicFireCandidate package is missing. See BUILD.md for the private fireball playtest.'
}
# Interactive player launch, using the existing DreamboundSegments profile.
Start-Process -FilePath $reverieExecutable -WorkingDirectory (Split-Path -Parent $reverieExecutable) -WindowStyle Normal
