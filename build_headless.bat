@echo off
echo Running S32DS headless build...
set ECLIPSE=C:\NXP\S32DS.3.6.5\eclipse\s32dsc.exe
set WORKSPACE=C:\Users\Charbel\workspaceS32DS.3.6.5
set PROJECT=Nxp_Cup_Pixy2_S32K144
set CONFIG=Debug_FLASH
set OUTFILE=%~dp0build_output.txt

"%ECLIPSE%" -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild ^
  -data "%WORKSPACE%" ^
  -build "%PROJECT%/%CONFIG%" ^
  > "%OUTFILE%" 2>&1

echo Build exit code: %ERRORLEVEL% >> "%OUTFILE%"
echo Done. Check build_output.txt
