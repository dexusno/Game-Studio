param(
 [ValidateSet('Editor','Shipping')][string]$Build='Editor',
 [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$OutputName='CreaturePerformance',
 [Parameter(Mandatory=$true)][string]$CaptureDirectory,
 [switch]$LegacyCreatureRig
)
$ErrorActionPreference='Stop'
$playerMotionGameRoot=Split-Path -Parent $PSScriptRoot
$playerMotionRepoRoot=[IO.Path]::GetFullPath((Join-Path $playerMotionGameRoot '../..'))
$playerMotionRoot=[IO.Path]::GetFullPath($CaptureDirectory)
if(Test-Path -LiteralPath (Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy')){
 throw 'Choose a new capture directory to preserve the previous study.'
}
New-Item -ItemType Directory -Path $playerMotionRoot -Force | Out-Null
$playerMotionArgs=@()
if($Build -eq 'Editor'){
 $playerMotionSettings=Get-Content -LiteralPath (Join-Path $playerMotionRepoRoot 'config.local.json') -Raw | ConvertFrom-Json
 $playerMotionExe=$playerMotionSettings.tools.unreal
 $playerMotionArgs+=('"'+(Join-Path $playerMotionGameRoot 'unreal/Dreambound.uproject')+'"')
 $playerMotionArgs+='-game'
}else{
 $playerMotionExe=Join-Path $playerMotionGameRoot ('BuildOutput/'+$OutputName+'/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe')
}
if(-not(Test-Path -LiteralPath $playerMotionExe -PathType Leaf)){throw 'Selected build is missing.'}
$playerMotionArgs+=@('-DBMotionFrameStudy','-DBSeed=552389','-DBSaveSlot=DreamboundQA_PlayerMotion',
 ('-UserDir="'+$playerMotionRoot+'/"'),('-abslog="'+$playerMotionRoot+'/engine.log"'),
 '-windowed','-ResX=1280','-ResY=800','-ForceRes','-RenderOffscreen','-NoSound','-NoVSync','-unattended','-nop4','-nosplash')
if($LegacyCreatureRig){$playerMotionArgs+='-DBLegacyCreatureRig'}
$playerMotionProcess=Start-Process -FilePath $playerMotionExe -ArgumentList $playerMotionArgs -WorkingDirectory (Split-Path -Parent $playerMotionExe) -WindowStyle Hidden -PassThru
$playerMotionIdentity=@{build=$Build;executable=$playerMotionExe;pid=$playerMotionProcess.Id;arguments=$playerMotionArgs;
 executable_sha256=(Get-FileHash -LiteralPath $playerMotionExe -Algorithm SHA256).Hash;captured_utc=(Get-Date).ToUniversalTime().ToString('o')}
if($Build -eq 'Editor'){
 $playerMotionIdentity.game_dll_sha256=(Get-FileHash -LiteralPath (Join-Path $playerMotionGameRoot 'unreal/Binaries/Win64/UnrealEditor-Dreambound.dll') -Algorithm SHA256).Hash
}
$playerMotionIdentity | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $playerMotionRoot 'Invocation.json') -Encoding utf8
Write-Output ('Started isolated '+$Build+' player motion study, PID '+$playerMotionProcess.Id)
$playerMotionProcess.WaitForExit()
if($playerMotionProcess.ExitCode -ne 0){throw ('Capture process exited with '+$playerMotionProcess.ExitCode)}
$playerMotionNote=Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy/Capture.txt'
if(Test-Path -LiteralPath (Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy/TimingFailure.txt')){
 throw (Get-Content -LiteralPath (Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy/TimingFailure.txt') -Raw)
}
if(-not(Test-Path -LiteralPath $playerMotionNote)){throw 'No completion note. Inspect engine.log.'}
$playerMotionRows=@(Import-Csv -LiteralPath (Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy/Frames.csv'))
$playerMotionPNGs=@(Get-ChildItem -LiteralPath (Join-Path $playerMotionRoot 'Saved/PlayerMotionStudy') -Filter 'Frame_*.png')
if($playerMotionRows.Count -ne $playerMotionPNGs.Count -or $playerMotionRows.Count -lt 1080){throw 'Frame study has missing samples or screenshots.'}
foreach($playerMotionRow in $playerMotionRows){
 $playerMotionDt=[double]::Parse($playerMotionRow.dt,[Globalization.CultureInfo]::InvariantCulture)
 if([Math]::Abs($playerMotionDt-1.0/30.0) -gt .0001){throw 'Recorded engine dt is not 30 Hz.'}
}
Get-Content -LiteralPath $playerMotionNote
