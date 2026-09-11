param(
 [ValidateSet('Editor','Shipping')][string]$Build='Editor',
 [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$OutputName='CreatureAnatomy2',
 [ValidateSet('Melee','Caster','Hunter','Boss')][string]$Creature='Melee',
 [ValidateSet('Side','LowSide','Front','Player','Detail','Impact')][string]$View='Side',
 [ValidateSet('Default','Performance','Anatomy','Dread','Legacy')][string]$Rig='Default',
 [ValidateSet('Lit','BaseColor','Roughness','Specular')][string]$Surface='Lit',
 [Parameter(Mandatory=$true)][string]$CaptureDirectory,
 [ValidateRange(640,2560)][int]$Width=1280,
 [ValidateRange(480,1600)][int]$Height=800,
 [ValidateRange(4,32)][int]$Seconds=32,
 [switch]$FootMarkers,
 [switch]$Idle,
 [switch]$WithAudio,
 [switch]$PreviewAudio,
 [switch]$PauseStudy
)
$ErrorActionPreference='Stop'
if($FootMarkers -and $Build -ne 'Editor'){throw 'Foot markers require an Editor capture.'}
if($Surface -ne 'Lit' -and $Build -ne 'Editor'){throw 'Surface diagnostics require an Editor capture.'}
if($PreviewAudio -and ($Build -ne 'Editor' -or -not $WithAudio)){throw 'Private audio requires an Editor capture with WithAudio.'}
if($PauseStudy -and -not $WithAudio){throw 'PauseStudy requires a real-time WithAudio capture.'}
$motionGameRoot=Split-Path -Parent $PSScriptRoot
$motionRepoRoot=[IO.Path]::GetFullPath((Join-Path $motionGameRoot '../..'))
$motionCaptureRoot=[IO.Path]::GetFullPath($CaptureDirectory)
if(Test-Path -LiteralPath (Join-Path $motionCaptureRoot 'Saved/EnemyMotionStudy')){
 throw 'Choose a new capture directory so the prior sequence is preserved.'
}
New-Item -ItemType Directory -Path $motionCaptureRoot -Force | Out-Null
$motionArguments=@()
if($Build -eq 'Editor'){
 $motionSettings=Get-Content -LiteralPath (Join-Path $motionRepoRoot 'config.local.json') -Raw | ConvertFrom-Json
 $motionExecutable=$motionSettings.tools.unreal
 $motionArguments+=('"'+(Join-Path $motionGameRoot 'unreal/Dreambound.uproject')+'"')
 $motionArguments+='-game'
}else{
 $motionExecutable=Join-Path $motionGameRoot ('BuildOutput/'+$OutputName+'/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe')
}
if(-not(Test-Path -LiteralPath $motionExecutable -PathType Leaf)){throw 'The selected build is missing.'}
$motionArguments+=@('-DBEnemyMotionStudy',('-DBCreature='+$Creature),('-DBCreatureView='+$View),'-DBSeed=552389',
 ('-DBCreatureStudySeconds='+$Seconds),
 '-DBSaveSlot=DreamboundQA_EnemyMotion',('-UserDir="'+$motionCaptureRoot+'/"'),('-abslog="'+$motionCaptureRoot+'/engine.log"'),
 '-windowed',('-ResX='+$Width),('-ResY='+$Height),'-ForceRes','-RenderOffscreen','-NoVSync','-unattended','-nop4','-nosplash')
if($WithAudio){$motionArguments+='-DBCreatureStudyAudio'}else{$motionArguments+='-NoSound'}
if($PreviewAudio){$motionArguments+='-DBOrganicFireAudioPreview'}
if($PauseStudy){$motionArguments+='-DBCreatureStudyPause'}
if($FootMarkers){$motionArguments+='-DBFootMarkers'}
if($Idle){$motionArguments+='-DBCreatureStudyIdle'}
if($Rig -eq 'Anatomy'){$motionArguments+='-DBAnatomyCreatureRig'}
if($Rig -eq 'Dread'){$motionArguments+='-DBDreadCreatureRig'}
if($Rig -eq 'Performance'){$motionArguments+='-DBPerformanceCreatureRig'}
if($Rig -eq 'Legacy'){$motionArguments+='-DBLegacyCreatureRig'}
if($Surface -ne 'Lit'){$motionArguments+=('-ExecCmds="viewmode VisualizeBuffer,r.BufferVisualizationTarget '+$Surface+'"')}
$motionProcess=Start-Process -FilePath $motionExecutable -ArgumentList $motionArguments -WorkingDirectory (Split-Path -Parent $motionExecutable) -WindowStyle Hidden -PassThru
$motionIdentity=@{build=$Build;creature=$Creature;view=$View;rig=$Rig;surface=$Surface;passive_idle=[bool]$Idle;audio=[bool]$WithAudio;private_audio_audition=[bool]$PreviewAudio;pause_study=[bool]$PauseStudy;pid=$motionProcess.Id;executable=$motionExecutable;
 executable_sha256=(Get-FileHash -LiteralPath $motionExecutable -Algorithm SHA256).Hash;
 captured_utc=(Get-Date).ToUniversalTime().ToString('o');arguments=$motionArguments}
if($Build -eq 'Editor'){
 $motionDll=Join-Path $motionGameRoot 'unreal/Binaries/Win64/UnrealEditor-Dreambound.dll'
 $motionIdentity.game_dll_sha256=(Get-FileHash -LiteralPath $motionDll -Algorithm SHA256).Hash
}
$motionIdentity | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $motionCaptureRoot 'Invocation.json') -Encoding utf8
Write-Output ('Started isolated '+$Build+' '+$Creature+' '+$View+' capture, PID '+$motionProcess.Id)
$motionProcess.WaitForExit()
if($motionProcess.ExitCode -ne 0){throw ('Capture process exited with code '+$motionProcess.ExitCode)}
$motionResultPath=Join-Path $motionCaptureRoot ('Saved/EnemyMotionStudy/'+$Creature+'-'+$View+'/Capture.json')
if(-not(Test-Path -LiteralPath $motionResultPath)){throw 'The process did not produce a capture report. Inspect its engine.log.'}
$motionResult=Get-Content -LiteralPath $motionResultPath -Raw | ConvertFrom-Json
if(-not $motionResult.complete -or $motionResult.aborted){throw 'The motion study was incomplete.'}
if($WithAudio){
 $motionMix=Join-Path (Split-Path -Parent $motionResultPath) 'Mix.wav'
 if(-not(Test-Path -LiteralPath $motionMix) -or (Get-Item -LiteralPath $motionMix).Length -lt 128){throw 'The real-time study did not export its audio mix.'}
}
if($Surface -ne 'Lit' -and (Select-String -LiteralPath (Join-Path $motionCaptureRoot 'engine.log') -Pattern 'view mode not recognized|Debug viewmodes not allowed' -Quiet)){
 throw 'The requested material diagnostic view was rejected; these frames are not a valid surface pass.'
}
Write-Output ('Complete: '+$motionResult.frames+' actual frames in '+$motionResult.simulation_seconds+' simulation seconds. '+$motionResultPath)
