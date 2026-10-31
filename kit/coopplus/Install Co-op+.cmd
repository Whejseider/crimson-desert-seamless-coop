@echo off
rem Runs the Co-op+ installer (install.ps1 next to this file).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
