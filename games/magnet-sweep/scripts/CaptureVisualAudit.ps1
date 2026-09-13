param(
 [string]$Executable='',
 [string]$OutputDirectory='',
 [ValidateRange(60,1800)][int]$TimeoutSeconds=300,
 [switch]$Extended
)
$ErrorActionPreference='Stop'
$probeRoot=Split-Path -Parent $PSScriptRoot
$probeRepo=[IO.Path]::GetFullPath((Join-Path $probeRoot '../..'))
$probeArgs=[Collections.Generic.List[string]]::new()
$probeEditorMode=[string]::IsNullOrWhiteSpace($Executable)
if([string]::IsNullOrWhiteSpace($Executable)){
 $probeConfig=Get-Content -LiteralPath (Join-Path $probeRepo 'config.local.json') -Raw | ConvertFrom-Json
 $Executable=$probeConfig.tools.unreal
 $probeArgs.Add('"'+(Join-Path $probeRoot 'unreal/MagnetSweep.uproject')+'"')
 $probeArgs.Add('-game')
}
$probeExecutable=[IO.Path]::GetFullPath($Executable)
if(!(Test-Path -LiteralPath $probeExecutable -PathType Leaf)){throw 'Capture executable does not exist'}
if([string]::IsNullOrWhiteSpace($OutputDirectory)){
 $OutputDirectory=Join-Path $probeRoot ('BuildOutput/VisualAudit/'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
}
$probeOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $probeOutput){throw 'Choose a new output directory; existing audit evidence is never overwritten'}
New-Item -ItemType Directory -Path $probeOutput | Out-Null
$probeProfile='visual_audit_'+[guid]::NewGuid().ToString('N').Substring(0,24)
foreach($probeArg in @('-Expedition','-VisualAudit',('-ExpeditionProfile='+$probeProfile),('-VisualAuditDir="'+$probeOutput+'"'),'-RenderOffScreen','-windowed','-ResX=1600','-ResY=900','-ForceRes','-unattended','-nosplash','-nosound','-NoVSync','-nop4','-ExecCmds="t.MaxFPS 30"',('-abslog="'+(Join-Path $probeOutput 'engine.log')+'"'))){$probeArgs.Add($probeArg)}
if($Extended){$probeArgs.Add('-VisualAuditExtended')}
$probeStart=Get-Date
$probeProcess=Start-Process -FilePath $probeExecutable -ArgumentList $probeArgs.ToArray() -WindowStyle Hidden -PassThru
$probeLaunch=[ordered]@{schema=1;pid=$probeProcess.Id;executable=$probeExecutable;arguments=$probeArgs.ToArray();started_utc=$probeStart.ToUniversalTime().ToString('o');timeout_seconds=$TimeoutSeconds;output=$probeOutput;status='running'}
$probeLaunch | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $probeOutput 'launch.json') -Encoding utf8
Write-Output ('Visual audit started: PID '+$probeProcess.Id+' / '+$probeOutput)
$probeLastProgress=Get-Date
while(!$probeProcess.HasExited){
 if(((Get-Date)-$probeStart).TotalSeconds -ge $TimeoutSeconds){
  # This is the exact process this script launched. No process-name kill or UI
  # operation can affect another running game/editor session.
  $probeProcess.Kill($true)
  $probeProcess.WaitForExit()
  $probeLaunch.status='timeout'
  $probeLaunch | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $probeOutput 'launch.json') -Encoding utf8
  throw ('Visual audit exceeded timeout; stopped only its launched process tree. Evidence: '+$probeOutput)
 }
 if(((Get-Date)-$probeLastProgress).TotalSeconds -ge 20){
  $probeLastProgress=Get-Date
  $probeReportPath=Join-Path $probeOutput 'report.json'
  if(Test-Path -LiteralPath $probeReportPath){
   try{$probeProgress=Get-Content -LiteralPath $probeReportPath -Raw | ConvertFrom-Json; Write-Output ('Visual audit stage '+$probeProgress.stage+' / '+$probeProgress.status)}catch{Write-Output 'Visual audit report is being written'}
  }else{Write-Output 'Visual audit engine startup still running'}
 }
 Start-Sleep -Seconds 1
 $probeProcess.Refresh()
}
$probeProcess.WaitForExit()
$probeLaunch.status='exited';$probeLaunch.exit_code=$probeProcess.ExitCode
$probeLaunch | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $probeOutput 'launch.json') -Encoding utf8
if($probeProcess.ExitCode -ne 0){throw ('Visual audit engine exited '+$probeProcess.ExitCode+'. Evidence: '+$probeOutput)}
$probeReportPath=Join-Path $probeOutput 'report.json'
if(!(Test-Path -LiteralPath $probeReportPath)){throw ('Engine exited without a visual audit report. Check that the probe hooks are compiled: '+$probeOutput)}
$probeReport=Get-Content -LiteralPath $probeReportPath -Raw | ConvertFrom-Json
$probeExpectedNames=@('01-starter.png','02-outfitter-selected.png','03-first-worksite.png')
if($Extended){$probeExpectedNames+=@('04-balanced-rack-fixture.png','05-counterweight-exchange-fixture.png')}
if($probeReport.status -ne 'complete' -or @($probeReport.captures).Count -ne $probeExpectedNames.Count -or [bool]$probeReport.extended -ne $Extended.IsPresent){throw ('Visual audit incomplete or wrong capture mode: '+$probeReport.error+' / '+$probeOutput)}
$probeHashes=@()
$probeCaptureIndex=0
foreach($probeCapture in $probeReport.captures){
 if($probeCapture.file -ne $probeExpectedNames[$probeCaptureIndex]){throw 'Visual audit returned an unexpected capture sequence'}
 if([bool]$probeCapture.unearned_late_site_fixture -ne ($probeCaptureIndex -ge 3)){throw 'Late-site fixture evidence was not labelled correctly'}
 $probeFile=[IO.Path]::GetFullPath((Join-Path $probeOutput $probeCapture.file))
 if([IO.Path]::GetDirectoryName($probeFile) -ne $probeOutput){throw 'Capture report referenced a file outside its audit directory'}
 if(!(Test-Path -LiteralPath $probeFile -PathType Leaf) -or (Get-Item -LiteralPath $probeFile).Length -le 32){throw 'Reported capture file is absent or empty'}
 $probeHashes += [ordered]@{file=$probeCapture.file;bytes=(Get-Item -LiteralPath $probeFile).Length;sha256=(Get-FileHash -LiteralPath $probeFile -Algorithm SHA256).Hash.ToLowerInvariant();width=$probeCapture.width;height=$probeCapture.height;unearned_late_site_fixture=[bool]$probeCapture.unearned_late_site_fixture;kind=$probeCapture.kind}
 ++$probeCaptureIndex
}
$probeModuleHash=$null
if($probeEditorMode){
 $probeModule=Join-Path $probeRoot 'unreal/Binaries/Win64/UnrealEditor-MagnetSweep.dll'
 if(Test-Path -LiteralPath $probeModule -PathType Leaf){$probeModuleHash=(Get-FileHash -LiteralPath $probeModule -Algorithm SHA256).Hash.ToLowerInvariant()}
}
[ordered]@{schema=1;captured_utc=(Get-Date).ToUniversalTime().ToString('o');extended=$Extended.IsPresent;executable_sha256=(Get-FileHash -LiteralPath $probeExecutable -Algorithm SHA256).Hash.ToLowerInvariant();editor_game_module_sha256=$probeModuleHash;captures=$probeHashes;limits='First three captures follow actual fresh-run callbacks. Optional late-site captures four and five are unearned presentation fixtures using StartSite(index,42) and the unchanged starting rig, without gear or reward injection. Engine rendering only; inspect images. This does not prove native input, earned progression, sound, performance or gameplay enjoyment.'} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $probeOutput 'capture-hashes.json') -Encoding utf8
Write-Output ('Visual audit complete: '+$probeReportPath)
