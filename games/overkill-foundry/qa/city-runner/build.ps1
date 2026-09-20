param([string]$SourceRoot = (Join-Path $PSScriptRoot '../..'), [string]$BuildName = 'native')
$ErrorActionPreference = 'Stop'
if ($BuildName -notmatch '^[a-z0-9-]+$') { throw 'Use a simple build directory name.' }
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$qaRoot = $PSScriptRoot
$build = Join-Path $qaRoot ('build/' + $BuildName)
$cmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
function Portable([object]$line) { ([string]$line).Replace($source,'<review-source>').Replace($source.Replace('\','/'),'<review-source>').Replace($qaRoot,'<qa/city-runner>').Replace($qaRoot.Replace('\','/'),'<qa/city-runner>') }
& $cmake -S $qaRoot -B $build -G 'Visual Studio 17 2022' -A x64 "-DOVERKILL_QA_SOURCE_ROOT=$source" | ForEach-Object { Portable $_ }
if ($LASTEXITCODE -ne 0) { throw 'QA configure failed.' }
& $cmake --build $build --config Release --parallel 4 | ForEach-Object { Portable $_ }
if ($LASTEXITCODE -ne 0) { throw 'QA build failed.' }
