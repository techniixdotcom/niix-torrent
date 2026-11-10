@echo off
rem Double-click me: asks whether to build the installer or upload the source code.
rem (Runs build.ps1 without having to change the PowerShell execution policy.)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
set RESULT=%ERRORLEVEL%
echo.
pause
exit /b %RESULT%
