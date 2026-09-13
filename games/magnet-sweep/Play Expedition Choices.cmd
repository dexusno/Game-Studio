@echo off
setlocal
set "magnet_expedition_choices=%~dp0BuildOutput\ExpeditionChoices\Windows\MagnetSweep.exe"
if not exist "%magnet_expedition_choices%" (
  echo The Expedition Choices build is not available on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_expedition_choices%" -Expedition -ExpeditionProfile=expedition_choices_preview
