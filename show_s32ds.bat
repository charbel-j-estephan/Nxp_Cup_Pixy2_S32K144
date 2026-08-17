@echo off
set OUT=%~dp0show_s32ds_out.txt
echo Checking for s32ds processes... > "%OUT%"
tasklist /fi "imagename eq s32ds.exe" >> "%OUT%" 2>&1
tasklist /fi "imagename eq s32dsc.exe" >> "%OUT%" 2>&1
echo --- window list --- >> "%OUT%"
powershell -Command "Get-Process s32ds -ErrorAction SilentlyContinue | Select-Object Id,MainWindowTitle,MainWindowHandle | Format-List" >> "%OUT%" 2>&1
echo --- >> "%OUT%"
powershell -Command "Add-Type -TypeDefinition 'using System;using System.Runtime.InteropServices;public class Win32{[DllImport(\"user32.dll\")]public static extern bool ShowWindow(IntPtr h,int n);[DllImport(\"user32.dll\")]public static extern bool SetForegroundWindow(IntPtr h);}'; $p=Get-Process s32ds -ErrorAction SilentlyContinue | Where-Object{$_.MainWindowHandle -ne 0} | Select-Object -First 1; if($p){[Win32]::ShowWindow($p.MainWindowHandle,9);[Win32]::SetForegroundWindow($p.MainWindowHandle); Write-Output 'S32DS brought to front'} else {Write-Output 'No S32DS window found'}" >> "%OUT%" 2>&1
echo Done.
