@echo off
setlocal
set "magnet_demo=%~dp0BuildOutput\Clarity\Windows\MagnetSweep.exe"
if not exist "%magnet_demo%" (
  echo The corrected Magnet Sweep demo has not been built on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_demo%"
