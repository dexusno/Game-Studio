$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path $PSScriptRoot -Parent
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source } else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmake = Join-Path $vsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
$buildRoot = Join-Path $gameRoot 'platform/build'
& $cmake -S (Join-Path $gameRoot 'platform') -B $buildRoot -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Storage configuration failed.' }
& $cmake --build $buildRoot --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Storage build failed.' }
& (Join-Path (Split-Path $cmake -Parent) 'ctest.exe') --test-dir $buildRoot -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Storage crash checks failed.' }
