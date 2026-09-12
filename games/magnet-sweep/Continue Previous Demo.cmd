@echo off
setlocal
set "magnet_demo=%~dp0BuildOutput\Rework\Windows\MagnetSweep.exe"
if not exist "%magnet_demo%" (
  echo The previous Magnet Sweep rebuild is not installed on this computer.
  pause
  exit /b 1
)
start "" "%magnet_demo%"
