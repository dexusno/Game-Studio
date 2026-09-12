@echo off
setlocal
set "magnet_preview=%~dp0BuildOutput\Extraction\Windows\MagnetSweep.exe"
if not exist "%magnet_preview%" (
  echo The Extraction Coil preview has not been built on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_preview%" -DemoProfile=extraction_preview
