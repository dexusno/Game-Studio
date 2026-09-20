$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source } else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmake = Join-Path $vsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
$buildRoot = Join-Path $gameRoot 'platform/build/qa-storage'
& $cmake -S $PSScriptRoot -B $buildRoot -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Independent storage probe configuration failed.' }
& $cmake --build $buildRoot --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Independent storage probe build failed.' }
& (Join-Path $buildRoot 'Release/storage_independent_probes.exe') (Join-Path $buildRoot 'artifacts') | Tee-Object -FilePath (Join-Path $buildRoot 'last-run.log')
if ($LASTEXITCODE -ne 0) { throw 'Independent storage probes found failures; see last-run.log.' }
