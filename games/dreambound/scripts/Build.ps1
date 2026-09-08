param([ValidateSet('Editor','Content','Package','All')][string]$Stage='All',
 [ValidateSet('Development','Shipping')][string]$PackageConfiguration='Shipping',
 [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$OutputName='ShieldStudy')
$ErrorActionPreference='Stop'
$betaGameRoot=Split-Path -Parent $PSScriptRoot
$betaRepoRoot=[IO.Path]::GetFullPath((Join-Path $betaGameRoot '../..'))
$betaSettings=Get-Content (Join-Path $betaRepoRoot 'config.local.json') -Raw | ConvertFrom-Json
$betaEditor=$betaSettings.tools.unreal
$betaEngineRoot=[IO.Path]::GetFullPath((Join-Path (Split-Path $betaEditor -Parent) '../../..'))
$betaProject=Join-Path $betaGameRoot 'unreal/Dreambound.uproject'
if($Stage -eq 'Editor' -or $Stage -eq 'All'){
 & (Join-Path $betaEngineRoot 'Engine/Build/BatchFiles/Build.bat') DreamboundEditor Win64 Development "-Project=$betaProject" -WaitMutex -NoHotReloadFromIDE
 if($LASTEXITCODE -ne 0){throw 'Unreal editor compilation failed'}
}
if($Stage -eq 'Content' -or $Stage -eq 'All'){
 & (Join-Path $betaEngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $betaProject -run=pythonscript "-script=$PSScriptRoot/CreateContent.py" -unattended -nop4 -nosplash -NullRHI
 if($LASTEXITCODE -ne 0){throw 'Content import failed'}
}
if($Stage -eq 'Package' -or $Stage -eq 'All'){
 & (Join-Path $betaEngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$betaProject" -nop4 -platform=Win64 "-clientconfig=$PackageConfiguration" -build -cook -stage -pak -iostore -nodebuginfo -archive "-archivedirectory=$betaGameRoot/BuildOutput/$OutputName" -utf8output
 if($LASTEXITCODE -ne 0){throw 'Unreal packaging failed'}
}
