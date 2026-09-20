param([string]$Name = 'candidate')
$ErrorActionPreference = 'Stop'
if ($Name -notmatch '^[a-z0-9-]+$') { throw 'Use a simple snapshot name.' }
$gameRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$destination = Join-Path $PSScriptRoot ('build/' + $Name)
if (Test-Path -LiteralPath $destination) { throw 'Snapshot already exists; preserve it.' }
$paths = @('core/CMakeLists.txt')
foreach ($directory in @('core/include','core/src','core/runner')) {
    Get-ChildItem -LiteralPath (Join-Path $gameRoot $directory) -File -Recurse | ForEach-Object {
        $paths += [IO.Path]::GetRelativePath($gameRoot,$_.FullName).Replace('\','/')
    }
}
$hashes = [ordered]@{}
foreach ($relative in $paths) { $hashes[$relative] = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $gameRoot $relative)).Hash.ToLowerInvariant() }
foreach ($relative in $paths) {
    $target = Join-Path $destination $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
    Copy-Item -LiteralPath (Join-Path $gameRoot $relative) -Destination $target
}
foreach ($relative in $paths) {
    foreach ($root in @($gameRoot,$destination)) {
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $relative)).Hash.ToLowerInvariant() -ne $hashes[$relative]) { throw ('Source changed while capturing: ' + $relative) }
    }
}
[ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); git_head=(& git rev-parse HEAD); sources=$hashes } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'identity.json') -Encoding UTF8
Write-Output ('CAPTURED ' + $Name + ' files=' + $paths.Count + ' before/copy/after equal')
