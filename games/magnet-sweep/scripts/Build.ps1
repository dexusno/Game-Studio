param([ValidateSet('Editor','Content','Package','Tests','All')][string]$Stage='All',
 [ValidateSet('Development','Shipping')][string]$Configuration='Development',
 [string]$ArchiveName='Tutorial')
$ErrorActionPreference='Stop'
$magnetRoot=Split-Path -Parent $PSScriptRoot
$magnetRepo=[IO.Path]::GetFullPath((Join-Path $magnetRoot '../..'))
$magnetConfig=Get-Content (Join-Path $magnetRepo 'config.local.json') -Raw | ConvertFrom-Json
$magnetEditor=$magnetConfig.tools.unreal
$magnetEngine=[IO.Path]::GetFullPath((Join-Path (Split-Path $magnetEditor -Parent) '../../..'))
$magnetProject=Join-Path $magnetRoot 'unreal/MagnetSweep.uproject'
if($Stage -in @('Editor','All')) {
 & (Join-Path $magnetEngine 'Engine/Build/BatchFiles/Build.bat') MagnetSweepEditor Win64 Development "-Project=$magnetProject" -WaitMutex -NoHotReloadFromIDE
 if($LASTEXITCODE -ne 0){throw 'Magnet Sweep editor compilation failed'}
}
if($Stage -in @('Content','All')) {
 $magnetContentReport=Join-Path $magnetRoot 'unreal/Saved/ConceptContent.json'
 if(Test-Path -LiteralPath $magnetContentReport){Remove-Item -LiteralPath $magnetContentReport}
 $magnetArgs=@(('"'+$magnetProject+'"'),('-ExecutePythonScript="'+$PSScriptRoot+'/CreateContent.py"'),'-unattended','-nop4','-nosplash','-NullRHI')
 $magnetProcess=Start-Process -FilePath $magnetEditor -ArgumentList $magnetArgs -WindowStyle Hidden -PassThru -Wait
 if($magnetProcess.ExitCode -ne 0){throw 'Magnet Sweep content build failed'}
 if(!(Test-Path -LiteralPath $magnetContentReport)){throw 'Content generation did not produce its completion report'}
 $magnetContent=Get-Content -LiteralPath $magnetContentReport -Raw | ConvertFrom-Json
 $magnetExpectedMeshes=@(Get-ChildItem -LiteralPath (Join-Path $magnetRoot 'assets/models') -Filter '*.fbx').Count
 $magnetExpectedAudio=@(Get-ChildItem -LiteralPath (Join-Path $magnetRoot 'assets/audio') -Filter '*.wav' -Recurse).Count
 if(!$magnetContent.complete -or @($magnetContent.imports).Count -ne ($magnetExpectedMeshes+$magnetExpectedAudio)){throw 'Content generation did not verify every source mesh and sound'}
}
if($Stage -eq 'Tests') {
 $magnetTestReport=Join-Path $magnetRoot 'unreal/Saved/Automation/index.json'
 if(Test-Path -LiteralPath $magnetTestReport){Remove-Item -LiteralPath $magnetTestReport}
 $magnetCmd=Join-Path (Split-Path $magnetEditor -Parent) 'UnrealEditor-Cmd.exe'
 & $magnetCmd $magnetProject -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests MagnetSweep' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$magnetRoot/unreal/Saved/Automation"
 if($LASTEXITCODE -ne 0){throw 'Magnet Sweep automation tests failed'}
 if(!(Test-Path -LiteralPath $magnetTestReport)){throw 'Automation did not export a fresh report'}
 $magnetResults=Get-Content -LiteralPath $magnetTestReport -Raw | ConvertFrom-Json
 if($magnetResults.failed -gt 0 -or $magnetResults.notRun -gt 0 -or $magnetResults.succeeded -lt 1){throw 'Magnet Sweep automation report contains failed, skipped or missing tests'}
}
if($Stage -in @('Package','All')) {
 if($ArchiveName -notmatch '^[a-zA-Z0-9_-]+$'){throw 'ArchiveName must be a simple directory name'}
 & (Join-Path $magnetEngine 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$magnetProject" -nop4 -platform=Win64 "-clientconfig=$Configuration" -build -cook -stage -pak -iostore -nodebuginfo -archive "-archivedirectory=$magnetRoot/BuildOutput/$ArchiveName" -utf8output
 if($LASTEXITCODE -ne 0){throw 'Magnet Sweep packaging failed'}
}
