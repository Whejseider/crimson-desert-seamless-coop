@echo off
rem Writes a Co-op+ report to the Desktop (diagnose.ps1 next to this file). Reads only.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0diagnose.ps1"
