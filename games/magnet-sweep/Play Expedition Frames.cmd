@echo off
setlocal
set "magnet_expedition_frames=%~dp0BuildOutput\ExpeditionFrames\Windows\MagnetSweep.exe"
if not exist "%magnet_expedition_frames%" (
  echo The Expedition Frames build is not available on this computer yet.
  echo See BUILD.md for the build commands.
  pause
  exit /b 1
)
start "" "%magnet_expedition_frames%" -Expedition -ExpeditionProfile=expedition_frames_preview
