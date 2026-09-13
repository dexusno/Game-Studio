@echo off
setlocal
set "magnet_expedition_visuals=%~dp0BuildOutput\ExpeditionVisuals\Windows\MagnetSweep.exe"
if not exist "%magnet_expedition_visuals%" (
  echo The Expedition Visuals build is not available on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_expedition_visuals%" -Expedition -ExpeditionProfile=expedition_visuals_preview
