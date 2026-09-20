[CmdletBinding()]
param(
    [ValidateSet('Build', 'BuildGame', 'Content', 'Art', 'MaraArt', 'OperatorArt', 'SceneryArt', 'BackdropArt', 'RosterArt', 'RosterVerify', 'RosterProbe', 'ArtProbe', 'CampaignProbe', 'CityProbe', 'Audio', 'AudioProbe', 'Teaching', 'Run', 'Smoke', 'Fixture', 'Package')]
    [string]$Action = 'Build',
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$ArchiveDirectory = '',
    [string]$CoreRoot = '',
    [string]$SavePath = '',
    [string]$CityTrace = '',
    [switch]$InspectSave,
    [int]$Width = 1600,
    [int]$Height = 900
)
$ErrorActionPreference = 'Stop'
$taskRepo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$taskGame = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskProjectDir = Join-Path $taskGame 'unreal'
$taskProject = Join-Path $taskProjectDir 'OverkillFoundry.uproject'
$taskConfigPath = Join-Path $taskRepo 'config.local.json'
if (-not (Test-Path -LiteralPath $taskConfigPath)) { throw 'Missing ignored config.local.json; configure tools.unreal with the installed UnrealEditor.exe.' }
$taskConfig = Get-Content -LiteralPath $taskConfigPath -Raw | ConvertFrom-Json
$taskEditor = $taskConfig.tools.unreal
if (-not $taskEditor -or -not (Test-Path -LiteralPath $taskEditor)) { throw 'Configured tools.unreal executable does not exist.' }
$taskEngine = Split-Path (Split-Path (Split-Path $taskEditor))
$taskLogs = Join-Path $taskProjectDir 'Saved/BuildLogs'
New-Item -ItemType Directory -Force -Path $taskLogs | Out-Null
$taskStamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$taskLog = Join-Path $taskLogs "$Action-$taskStamp.log"

function Invoke-FoundryProcess([string]$Executable, [string[]]$Arguments) {
    $taskVerifyBinary = $Action -in @('CampaignProbe', 'CityProbe', 'ArtProbe', 'RosterProbe', 'Smoke', 'Fixture')
    $taskModulePath = Join-Path $taskProjectDir 'Binaries/Win64/UnrealEditor-OverkillFoundry.dll'
    if ($taskVerifyBinary) { $taskModuleBefore = (Get-FileHash -LiteralPath $taskModulePath -Algorithm SHA256).Hash }
    # ArgumentList is joined by Windows Start-Process; quote each path safely.
    $taskQuoted = $Arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }
    $taskProc = Start-Process -FilePath $Executable -ArgumentList $taskQuoted -WorkingDirectory $taskProjectDir -WindowStyle Hidden -PassThru
    $taskProc.WaitForExit()
    if ($taskVerifyBinary) {
        $taskModuleAfter = (Get-FileHash -LiteralPath $taskModulePath -Algorithm SHA256).Hash
        @{ module = 'UnrealEditor-OverkillFoundry.dll'; sha256Before = $taskModuleBefore; sha256After = $taskModuleAfter; log = [IO.Path]::GetFileName($taskLog); exitCode = $taskProc.ExitCode } |
            ConvertTo-Json | Set-Content -LiteralPath "$taskLog.binary.json" -Encoding utf8
        if ($taskModuleBefore -ne $taskModuleAfter) { throw "Module identity changed during $Action." }
    }
    if ($taskProc.ExitCode -ne 0) { throw "$Action failed with exit code $($taskProc.ExitCode). Inspect $taskLog" }
}

$taskPreviousCoreRoot = $env:FOUNDRY_CORE_ROOT
try {
if ($CoreRoot) {
    $taskCoreRoot = [IO.Path]::GetFullPath($CoreRoot)
    if (-not (Test-Path -LiteralPath (Join-Path $taskCoreRoot 'include/overkill/core.hpp'))) { throw 'CoreRoot must name an existing local core source snapshot.' }
    $env:FOUNDRY_CORE_ROOT = $taskCoreRoot
    Write-Output "Explicit review core source: $taskCoreRoot"
} else { $env:FOUNDRY_CORE_ROOT = $null }
if ($Action -in @('Build', 'BuildGame', 'Package')) {
    # UBT caches module rules. This tracked local dependency explicitly
    # invalidates that cache when switching between review and current sources.
    $taskCoreIdentity = if ($CoreRoot) { $taskCoreRoot } else { [IO.Path]::GetFullPath((Join-Path $taskGame 'core')) }
    $taskCoreStamp = Join-Path $taskProjectDir 'Saved/BuildCoreRoot.txt'
    if (-not (Test-Path -LiteralPath $taskCoreStamp) -or (Get-Content -LiteralPath $taskCoreStamp -Raw).Trim() -ne $taskCoreIdentity) {
        Set-Content -LiteralPath $taskCoreStamp -Value $taskCoreIdentity -NoNewline
    }
}
switch ($Action) {
    'Build' {
        & (Join-Path $taskEngine 'Build/BatchFiles/Build.bat') OverkillFoundryEditor Win64 Development "-Project=$taskProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles "-Log=$taskLog"
        if ($LASTEXITCODE -ne 0) { throw "Editor build failed: $LASTEXITCODE; $taskLog" }
    }
    'BuildGame' {
        & (Join-Path $taskEngine 'Build/BatchFiles/Build.bat') OverkillFoundry Win64 $Configuration "-Project=$taskProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles "-Log=$taskLog"
        if ($LASTEXITCODE -ne 0) { throw "Game build failed: $LASTEXITCODE; $taskLog" }
    }
    'Content' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/build_smoke_content.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-nullrhi', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_CONTENT_READY' -Quiet)) { throw "Content generation did not report success: $taskLog" }
    }
    'Run' {
        $taskArgs = @("`"$taskProject`"", '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', "`"-abslog=$taskLog`"")
        if ($SavePath) { $taskArgs += "`"-FoundrySave=$([IO.Path]::GetFullPath($SavePath))`"" }
        if ($InspectSave) {
            if (-not $SavePath) { throw 'InspectSave requires an explicit prepared fixture SavePath.' }
            $taskArgs += '-FoundryInspectSave'
        }
        $taskProc = Start-Process -FilePath $taskEditor -ArgumentList $taskArgs -WorkingDirectory $taskProjectDir -WindowStyle Normal -PassThru
        @{ module = 'UnrealEditor-OverkillFoundry.dll'; sha256Before = (Get-FileHash -LiteralPath (Join-Path $taskProjectDir 'Binaries/Win64/UnrealEditor-OverkillFoundry.dll') -Algorithm SHA256).Hash; log = [IO.Path]::GetFileName($taskLog); pid = $taskProc.Id } |
            ConvertTo-Json | Set-Content -LiteralPath "$taskLog.binary.json" -Encoding utf8
        Write-Output "Interactive host PID=$($taskProc.Id)"
    }
    'Teaching' {
        $taskArgs = @("`"$taskProject`"", '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-FoundryTeaching', "`"-abslog=$taskLog`"")
        $taskProc = Start-Process -FilePath $taskEditor -ArgumentList $taskArgs -WorkingDirectory $taskProjectDir -WindowStyle Normal -PassThru
        Write-Output "Interactive teaching PID=$($taskProc.Id)"
    }
    'Audio' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_audio.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-nullrhi', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_AUDIO_IMPORTED 23' -Quiet)) { throw "Audio import did not report success: $taskLog" }
    }
    'AudioProbe' {
        $taskAudioModule = Join-Path $taskProjectDir 'Binaries/Win64/UnrealEditor-OverkillFoundry.dll'
        $taskAudioBefore = (Get-FileHash -LiteralPath $taskAudioModule -Algorithm SHA256).Hash
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-FoundryAudioProbe', "-abslog=$taskLog")
        $taskAudioAfter = (Get-FileHash -LiteralPath $taskAudioModule -Algorithm SHA256).Hash
        @{ module = 'UnrealEditor-OverkillFoundry.dll'; sha256Before = $taskAudioBefore; sha256After = $taskAudioAfter; log = [IO.Path]::GetFileName($taskLog) } |
            ConvertTo-Json | Set-Content -LiteralPath "$taskLog.binary.json" -Encoding utf8
        if ($taskAudioBefore -ne $taskAudioAfter) { throw 'Module identity changed during AudioProbe.' }
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_AUDIO_CAPTURE' -Quiet)) { throw "Audio probe did not export a master mix: $taskLog" }
    }
    'CampaignProbe' {
        if (-not $SavePath) { $SavePath = Join-Path $taskProjectDir "Saved/CampaignProbes/$taskStamp/profile.ofsave" }
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundryCampaignProbe', '-FoundryTransactionTiming', "-FoundrySave=$([IO.Path]::GetFullPath($SavePath))", "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_CAMPAIGN_PROBE_COMPLETE ok=1' -Quiet)) { throw "Campaign probe did not report success: $taskLog" }
    }
    'CityProbe' {
        if (-not $CityTrace -or -not (Test-Path -LiteralPath $CityTrace -PathType Leaf)) { throw 'CityProbe requires an existing earned OFCITY2 trace in CityTrace.' }
        $taskCityRoot = Join-Path $taskProjectDir "Saved/CityProbe/render-$taskStamp"
        if (-not $SavePath) { $SavePath = Join-Path $taskCityRoot 'campaign.ofsave' }
        if (Test-Path -LiteralPath $SavePath) { throw 'CityProbe requires a fresh isolated save path.' }
        $taskCityOutput = Join-Path $taskCityRoot 'evidence'
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundryCampaignProbe', '-FoundryTransactionTiming', "-FoundryCityTrace=$([IO.Path]::GetFullPath($CityTrace))", "-FoundrySave=$([IO.Path]::GetFullPath($SavePath))", "-FoundryCityOutput=$taskCityOutput", "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -Pattern 'FOUNDRY_CITY_PROBE_COMPLETE ok=1 commands=257 captures=\d+ hash=241e46199e519eef position=13 hp=80 revision=258' -Quiet)) { throw "City replay did not report the expected earned witness: $taskLog" }
    }
    'Art' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_cinderwall.py'
        # Static-mesh build settings only rebuild render bounds with rendering enabled.
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_CINDERWALL_READY' -Quiet)) { throw "Cinderwall import did not report success: $taskLog" }
    }
    'ArtProbe' {
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundryArtProbe', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_ART_PROBE_COMPLETE ok=1' -Quiet)) { throw "Rendered art probe did not report success: $taskLog" }
    }
    'SceneryArt' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_scenery_v002.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_SCENERY_V002_READY' -Quiet)) { throw "Scenery import did not report success: $taskLog" }
    }
    'RosterArt' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_roster_v001.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_ROSTER_READY' -Quiet)) { throw "Roster import did not report success: $taskLog" }
    }
    'BackdropArt' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_backdrop_v001.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_BACKDROP_V001_READY' -Quiet)) { throw "Backdrop import did not report success: $taskLog" }
    }
    'RosterProbe' {
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundryRosterProbe', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_ROSTER_PROBE_COMPLETE ok=1' -Quiet)) { throw "Rendered roster probe did not report success: $taskLog" }
    }
    'RosterVerify' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_roster_v001.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-FoundryRosterVerify', '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_ROSTER_VERIFIED' -Quiet)) { throw "Roster reload verification did not report success: $taskLog" }
    }
    'MaraArt' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_mara.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_MARA_READY' -Quiet)) { throw "Mara import did not report success: $taskLog" }
    }
    'OperatorArt' {
        $taskCmdEditor = Join-Path (Split-Path $taskEditor) 'UnrealEditor-Cmd.exe'
        $taskScript = Join-Path $taskProjectDir 'Tools/import_operator_v001.py'
        Invoke-FoundryProcess $taskCmdEditor @($taskProject, '-run=pythonscript', "-script=$taskScript", '-unattended', '-nop4', '-nosplash', '-AllowCommandletRendering', '-asyncStaticMeshCompilation=0', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_OPERATOR_READY' -Quiet)) { throw "Operator import did not report success: $taskLog" }
    }
    'Smoke' {
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundrySmoke', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_HOST_SMOKE_COMPLETE' -Quiet)) { throw "Rendered host smoke did not report success: $taskLog" }
    }
    'Fixture' {
        Invoke-FoundryProcess $taskEditor @($taskProject, '-game', '-windowed', "-ResX=$Width", "-ResY=$Height", '-nosplash', '-nosound', '-FoundrySmoke', '-FoundryCoreFixture', '-FoundryInputProbe', "-abslog=$taskLog")
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_CORE_FIXTURE ok=1' -Quiet)) { throw "Shared core fixture did not report success: $taskLog" }
        if (-not (Select-String -LiteralPath $taskLog -SimpleMatch 'FOUNDRY_UI_INPUT_PROBE ok=1' -Quiet)) { throw "Presentation command-path probe did not report success: $taskLog" }
        Write-Output "Parity transcript: $(Join-Path $taskProjectDir 'Saved/Parity/core-fixture.jsonl')"
    }
    'Package' {
        if (-not $ArchiveDirectory) { $ArchiveDirectory = Join-Path $taskRepo '.local/builds/overkill-foundry' }
        & (Join-Path $taskEngine 'Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$taskProject" -noP4 -platform=Win64 "-clientconfig=$Configuration" -build -cook -stage -pak -iostore -archive "-archivedirectory=$ArchiveDirectory" -utf8output 2>&1 | Tee-Object -FilePath $taskLog
        if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE; $taskLog" }
    }
}
Write-Output "$Action completed. Log: $taskLog"
} finally { $env:FOUNDRY_CORE_ROOT = $taskPreviousCoreRoot }
