@echo off
setlocal
set "magnet_expedition_sites=%~dp0BuildOutput\ExpeditionSites\Windows\MagnetSweep.exe"
if not exist "%magnet_expedition_sites%" (
  echo The Expedition Sites build is not available on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_expedition_sites%" -Expedition -ExpeditionProfile=expedition_sites_preview
