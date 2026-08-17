@echo off
set OUT=%~dp0find_eclipse_out.txt
echo Searching for eclipse... > "%OUT%"
dir "C:\NXP\S32DS.3.6.5\" /b >> "%OUT%" 2>&1
echo --- >> "%OUT%"
dir "C:\NXP\S32DS.3.6.5\eclipse\" /b >> "%OUT%" 2>&1
echo --- >> "%OUT%"
type "C:\NXP\S32DS.3.6.5\s32ds.bat" >> "%OUT%" 2>&1
echo Done.
