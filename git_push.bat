@echo off
cd /d "C:\Users\Charbel\workspaceS32DS.3.6.5\Nxp_Cup_Pixy2_S32K144"
git add -A
git commit -m "feat: FreeMASTER USB+TSA integration (servo, ESC, brushless, battery, pixy2, button)"
git push
echo.
echo === DONE ===
pause
