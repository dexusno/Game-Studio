param([string]$SourceRoot = (Join-Path $PSScriptRoot '../../..'))
$ErrorActionPreference = 'Stop'
$qaRoot = $PSScriptRoot
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$qaBuild = Join-Path $qaRoot 'build/native'
$qaCmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
function PortableLine([object]$value) {
    ([string]$value).Replace($source, '<review-source>').Replace($source.Replace('\','/'), '<review-source>').Replace($qaRoot, '<qa/upgrades/campaign>').Replace($qaRoot.Replace('\','/'), '<qa/upgrades/campaign>')
}
$inputPaths = @('core/CMakeLists.txt')
foreach ($directory in @('core/include','core/src','platform/include','platform/src')) {
    Get-ChildItem -LiteralPath (Join-Path $source $directory) -Recurse -File | ForEach-Object { $inputPaths += [IO.Path]::GetRelativePath($source,$_.FullName).Replace('\','/') }
}
$before = [ordered]@{}
foreach ($relative in $inputPaths) { $before[$relative] = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $source $relative)).Hash.ToLowerInvariant() }
& $qaCmake -S $qaRoot -B $qaBuild -G 'Visual Studio 17 2022' -A x64 "-DOVERKILL_QA_SOURCE_ROOT=$source" | ForEach-Object { PortableLine $_ }
if ($LASTEXITCODE -ne 0) { throw 'Independent upgrade campaign configure failed.' }
& $qaCmake --build $qaBuild --config Release --parallel 4 | ForEach-Object { PortableLine $_ }
if ($LASTEXITCODE -ne 0) { throw 'Independent upgrade campaign build failed.' }
$qaExecutable = Join-Path $qaBuild 'Release/upgrade_campaign_probes.exe'
& $qaExecutable | ForEach-Object { PortableLine $_ } | Tee-Object -FilePath (Join-Path $qaBuild 'runtime-results.txt')
$runtimeCode = $LASTEXITCODE
$unchanged = $true
foreach ($relative in $inputPaths) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $source $relative)).Hash.ToLowerInvariant()
    if ($actual -ne $before[$relative]) { $unchanged = $false; Write-Warning ('Changed during run: ' + $relative) }
}
[ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    git_head_at_execution = (& git rev-parse HEAD)
    platform = 'Windows x64 Release / MSVC 19.44.35228 / SDK 10.0.26100.0'
    executable_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $qaExecutable).Hash.ToLowerInvariant()
    probe_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $qaRoot 'probes.cpp')).Hash.ToLowerInvariant()
    cmake_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $qaRoot 'CMakeLists.txt')).Hash.ToLowerInvariant()
    runner_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()
    source_unchanged_during_run = $unchanged
    runtime_exit = $runtimeCode
    source_sha256 = $before
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $qaBuild 'identity.json') -Encoding UTF8
Get-Content -LiteralPath (Join-Path $qaBuild 'identity.json')
if (-not $unchanged) { throw 'Review source changed during execution.' }
exit $runtimeCode
