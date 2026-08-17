@echo off
REM Kill any stuck s32dsc.exe headless build
taskkill /IM s32dsc.exe /F 2>nul

REM Launch run_make.sh in Git Bash
"C:\Program Files\Git\bin\bash.exe" "%~dp0run_make.sh"

echo.
echo Done. Build output is in build_output.txt
pause
