@echo off
setlocal
echo =============================================================
echo  NXP Cup S32K144 – Build + Git commit
echo =============================================================
echo.

REM ── 1. Headless Eclipse build ─────────────────────────────────
set ECLIPSE=C:\NXP\S32DS.3.6.5\eclipse\s32dsc.exe
set WORKSPACE=C:\Users\Charbel\workspaceS32DS.3.6.5
set PROJECT=Nxp_Cup_Pixy2_S32K144
set CONFIG=Debug_FLASH
set OUTFILE=%~dp0build_output.txt

echo [1/3] Building %PROJECT%/%CONFIG% (headless Eclipse)...
echo.
"%ECLIPSE%" -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild ^
  -data "%WORKSPACE%" ^
  -build "%PROJECT%/%CONFIG%" ^
  > "%OUTFILE%" 2>&1
set BUILD_EXIT=%ERRORLEVEL%
type "%OUTFILE%"
echo.
echo Build exit code: %BUILD_EXIT%
echo.
if %BUILD_EXIT% NEQ 0 (
    echo ERROR: Build failed. Fix the errors above, then re-run this script.
    pause
    exit /b %BUILD_EXIT%
)
echo Build SUCCEEDED.
echo.

REM ── 2. Git add + commit + push ────────────────────────────────
cd /d "%~dp0"
echo [2/3] Staging all changes...
git add -A
if %ERRORLEVEL% NEQ 0 ( echo git add failed. & pause & exit /b 1 )

echo [3/3] Committing...
git commit -m "feat: FreeMASTER USB+TSA integration (servo, ESC, brushless, battery, pixy2, button)"
if %ERRORLEVEL% NEQ 0 (
    echo git commit failed ^(nothing to commit, or config missing^).
    echo If "nothing to commit" is expected, that is fine.
)

echo Pushing to remote...
git push
if %ERRORLEVEL% NEQ 0 ( echo git push failed. & pause & exit /b 1 )

echo.
echo =============================================================
echo  All done! Build clean, committed and pushed.
echo =============================================================
pause
