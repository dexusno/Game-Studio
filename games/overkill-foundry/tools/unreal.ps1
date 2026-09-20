[CmdletBinding()]
param(
    [ValidateSet('Build', 'BuildGame', 'Content', 'Run', 'Smoke', 'Fixture', 'Package')]
    [string]$Action = 'Build',
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$ArchiveDirectory = '',
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
    # ArgumentList is joined by Windows Start-Process; quote each path safely.
    $taskQuoted = $Arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }
    $taskProc = Start-Process -FilePath $Executable -ArgumentList $taskQuoted -WorkingDirectory $taskProjectDir -WindowStyle Hidden -PassThru
    $taskProc.WaitForExit()
    if ($taskProc.ExitCode -ne 0) { throw "$Action failed with exit code $($taskProc.ExitCode). Inspect $taskLog" }
}

switch ($Action) {
    'Build' {
        & (Join-Path $taskEngine 'Build/BatchFiles/Build.bat') OverkillFoundryEditor Win64 Development "-Project=$taskProject" -WaitMutex -NoHotReloadFromIDE "-Log=$taskLog"
        if ($LASTEXITCODE -ne 0) { throw "Editor build failed: $LASTEXITCODE; $taskLog" }
    }
    'BuildGame' {
        & (Join-Path $taskEngine 'Build/BatchFiles/Build.bat') OverkillFoundry Win64 $Configuration "-Project=$taskProject" -WaitMutex -NoHotReloadFromIDE "-Log=$taskLog"
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
        $taskProc = Start-Process -FilePath $taskEditor -ArgumentList $taskArgs -WorkingDirectory $taskProjectDir -WindowStyle Normal -PassThru
        Write-Output "Interactive host PID=$($taskProc.Id)"
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
