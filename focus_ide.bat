@echo off
set OUT=%~dp0focus_ide_out.txt
echo Focusing IDE window (PID 10704)... > "%OUT%"
powershell -Command "Add-Type -TypeDefinition 'using System;using System.Runtime.InteropServices;public class Win32{[DllImport(\"user32.dll\")]public static extern bool ShowWindow(IntPtr h,int n);[DllImport(\"user32.dll\")]public static extern bool SetForegroundWindow(IntPtr h);}'; $p=Get-Process -Id 10704 -ErrorAction SilentlyContinue; if($p -and $p.MainWindowHandle -ne 0){[Win32]::ShowWindow($p.MainWindowHandle,9);[Win32]::SetForegroundWindow($p.MainWindowHandle); Write-Output 'IDE PID 10704 brought to front'} else {Write-Output 'PID 10704 not found or no window'}" >> "%OUT%" 2>&1
echo Done.
