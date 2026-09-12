@echo off
setlocal
set "magnet_expedition=%~dp0BuildOutput\Expedition\Windows\MagnetSweep.exe"
if not exist "%magnet_expedition%" (
  echo The Expedition Lab has not been built on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_expedition%" -Expedition -ExpeditionProfile=expedition_preview
