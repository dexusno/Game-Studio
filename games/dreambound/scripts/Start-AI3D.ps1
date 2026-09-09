param(
    [ValidateSet('Trellis', 'TrellisTrial', 'Hunyuan', 'Authenticate')]
    [string]$Tool = 'Trellis',
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ToolArguments
)

$ErrorActionPreference = 'Stop'
$studioRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$configPath = Join-Path $studioRoot 'config.local.json'
if (-not (Test-Path -LiteralPath $configPath)) {
    throw 'The local AI 3D installation is not configured. See games/dreambound/AI3D.md.'
}
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
$ai3d = $config.tools.ai3d
if (-not $ai3d.wslDistro -or -not $ai3d.wslRoot) {
    throw 'config.local.json needs tools.ai3d.wslDistro and tools.ai3d.wslRoot.'
}
$entry = if ($Tool -eq 'Hunyuan') { 'hunyuan-run' } else { 'trellis-run' }
$entryPath = $ai3d.wslRoot.TrimEnd('/') + '/' + $entry
if ($Tool -eq 'Authenticate') {
    & wsl.exe -d $ai3d.wslDistro --exec bash $entryPath --auth
} elseif ($Tool -eq 'TrellisTrial') {
    $trialScript = Join-Path $PSScriptRoot 'trellis_trial.py'
    $wslScript = & wsl.exe -d $ai3d.wslDistro --exec wslpath -a -u $trialScript
    if ($LASTEXITCODE -ne 0) { throw 'Could not translate the trial script path for WSL.' }
    & wsl.exe -d $ai3d.wslDistro --exec bash $entryPath --trial $wslScript.Trim() @ToolArguments
} else {
    & wsl.exe -d $ai3d.wslDistro --exec bash $entryPath @ToolArguments
}
exit $LASTEXITCODE
