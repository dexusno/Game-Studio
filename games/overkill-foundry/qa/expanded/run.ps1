$ErrorActionPreference = 'Stop'
$qaSource = $PSScriptRoot
$gameSource = (Resolve-Path -LiteralPath (Join-Path $qaSource '../..')).Path
$qaBuild = Join-Path $qaSource 'build'
$qaCmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
function PortableLine([object]$value) {
    ([string]$value).Replace($gameSource, '<game>').Replace($gameSource.Replace('\', '/'), '<game>')
}
$identityPaths = @(
    'core/include/overkill/core.hpp', 'core/include/overkill/robots.hpp',
    'core/src/core.cpp', 'core/src/serialization.cpp', 'core/src/recipe_effects.inl',
    'core/src/catalogue.cpp', 'core/src/catalogue_data.inc', 'core/src/robots.cpp',
    'content/cinderwall.manifest.json', 'qa/expanded/probes.cpp',
    'qa/expanded/CMakeLists.txt', 'qa/expanded/source_metadata.py'
)
$before = [ordered]@{}
foreach ($relative in $identityPaths) {
    $before[$relative] = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $gameSource $relative)).Hash.ToLowerInvariant()
}
& $qaCmake -S $qaSource -B $qaBuild -G 'Visual Studio 17 2022' -A x64 | ForEach-Object { PortableLine $_ }
if ($LASTEXITCODE -ne 0) { throw 'Independent configure failed.' }
& $qaCmake --build $qaBuild --config Release --parallel 4 | ForEach-Object { PortableLine $_ }
if ($LASTEXITCODE -ne 0) { throw 'Independent build failed.' }
$qaExecutable = Join-Path $qaBuild 'Release/expanded_probes.exe'
& $qaExecutable | ForEach-Object { PortableLine $_ } | Tee-Object -FilePath (Join-Path $qaBuild 'runtime-results.txt')
$runtimeCode = $LASTEXITCODE
& python (Join-Path $qaSource 'source_metadata.py') | ForEach-Object { PortableLine $_ } | Tee-Object -FilePath (Join-Path $qaBuild 'metadata-results.txt')
$metadataCode = $LASTEXITCODE
$unchanged = $true
foreach ($relative in $identityPaths) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $gameSource $relative)).Hash.ToLowerInvariant()
    if ($actual -ne $before[$relative]) { $unchanged = $false; Write-Warning ('Changed during run: ' + $relative) }
}
$identity = [ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    git_head = (& git rev-parse HEAD)
    platform = 'Windows x64 Release / MSVC 19.44.35228 / SDK 10.0.26100.0'
    executable_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $qaExecutable).Hash.ToLowerInvariant()
    source_unchanged_during_run = $unchanged
    runtime_exit = $runtimeCode
    metadata_exit = $metadataCode
    source_sha256 = $before
}
$identity | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $qaBuild 'identity.json') -Encoding UTF8
$identity | ConvertTo-Json -Depth 5
if (-not $unchanged) { throw 'Source changed during execution; no frozen-artifact result.' }
if ($runtimeCode -ne 0 -or $metadataCode -ne 0) { exit 1 }
