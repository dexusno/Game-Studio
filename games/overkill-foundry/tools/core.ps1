param([ValidateSet('build','test','fixture')][string]$Action = 'test', [ValidateSet('Debug','Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path $PSScriptRoot -Parent
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source } else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'CMake or Visual Studio with C++ CMake tools is required.' }
    $vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmake = Join-Path $vsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
if (!(Test-Path -LiteralPath $cmake)) { throw 'CMake executable was not found.' }
$buildRoot = Join-Path $gameRoot 'core/build'
& $cmake -S (Join-Path $gameRoot 'core') -B $buildRoot -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build $buildRoot --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Core build failed.' }
if ($Action -eq 'test') {
    & (Join-Path (Split-Path $cmake -Parent) 'ctest.exe') --test-dir $buildRoot -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Core behavioral checks failed.' }
} elseif ($Action -eq 'fixture') {
    & (Join-Path $buildRoot "$Configuration/overkill_runner.exe") --fixture
    if ($LASTEXITCODE -ne 0) { throw 'Teaching fixture failed.' }
}
