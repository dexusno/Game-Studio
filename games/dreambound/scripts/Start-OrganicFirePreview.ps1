param([switch]$CheckOnly)
$ErrorActionPreference='Stop'
$fireGameRoot=Split-Path -Parent $PSScriptRoot
$fireRepoRoot=[IO.Path]::GetFullPath((Join-Path $fireGameRoot '../..'))
$firePackage=Join-Path $fireGameRoot 'BuildOutput/OrganicFireCombat/Windows/Dreambound.exe'
$fireUserRoot=Join-Path $fireRepoRoot '.local/organic-fire-review/playable-preview'
if(Test-Path -LiteralPath $firePackage -PathType Leaf){
 $firePackageArguments=@('-DBSaveSlot=DreamboundOrganicFirePreview',('-UserDir="'+$fireUserRoot+'/"'),'-windowed','-ResX=1600','-ResY=1000')
 if($CheckOnly){
  [pscustomobject]@{executable=$firePackage;arguments=$firePackageArguments;kind='packaged-private-playtest'}
  return
 }
 New-Item -ItemType Directory -Path $fireUserRoot -Force | Out-Null
 Start-Process -FilePath $firePackage -ArgumentList $firePackageArguments -WorkingDirectory (Split-Path -Parent $firePackage) -WindowStyle Normal
 return
}
$fireSettings=Get-Content -LiteralPath (Join-Path $fireRepoRoot 'config.local.json') -Raw | ConvertFrom-Json
$fireExecutable=$fireSettings.tools.unreal
$fireProject=Join-Path $fireGameRoot 'unreal/Dreambound.uproject'
$fireDll=Join-Path $fireGameRoot 'unreal/Binaries/Win64/UnrealEditor-Dreambound.dll'
foreach($fireRequired in @($fireExecutable,$fireProject,$fireDll)){
 if(-not $fireRequired -or -not(Test-Path -LiteralPath $fireRequired -PathType Leaf)){
  throw 'The local Editor preview is missing its engine, project or compiled game module. See BUILD.md.'
 }
}
$fireCueNames=@('S_CasterIgnite','S_CasterFurnaceLoop','S_CasterReleaseA','S_CasterReleaseB','S_CasterReleaseC','S_CasterFlightLoop','S_CasterImpactA','S_CasterImpactB')
foreach($fireCue in $fireCueNames){
 $fireAsset=Join-Path $fireGameRoot ('unreal/Content/Audio/OrganicFire/'+$fireCue+'.uasset')
 if(-not(Test-Path -LiteralPath $fireAsset -PathType Leaf)){
  throw ('Private audition asset missing: '+$fireCue+'. See BUILD.md before launching.')
 }
}
$fireArguments=@(('"'+$fireProject+'"'),'-game','-DBDreadCreatureRig',
 '-DBSaveSlot=DreamboundOrganicFirePreview',('-UserDir="'+$fireUserRoot+'/"'),
 '-windowed','-ResX=1600','-ResY=1000','-nosplash')
if($CheckOnly){
 [pscustomobject]@{executable=$fireExecutable;arguments=$fireArguments;private_audio_assets=$fireCueNames.Count;game_dll_sha256=(Get-FileHash -LiteralPath $fireDll -Algorithm SHA256).Hash}
 return
}
New-Item -ItemType Directory -Path $fireUserRoot -Force | Out-Null
# Interactive local audition. This separate profile starts at the normal menu.
Start-Process -FilePath $fireExecutable -ArgumentList $fireArguments -WorkingDirectory (Split-Path -Parent $fireExecutable) -WindowStyle Normal
