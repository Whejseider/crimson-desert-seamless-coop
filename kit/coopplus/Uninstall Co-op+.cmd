@echo off
rem Removes Co-op+ from the game folder (uninstall.ps1 next to this file).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1"
