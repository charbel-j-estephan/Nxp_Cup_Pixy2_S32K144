Set oShell = CreateObject("WScript.Shell")
oShell.Run """C:\Program Files\Git\bin\bash.exe"" """ & WScript.ScriptFullName & """\..\run_make.sh""", 1, True
WScript.Echo "Build complete - check build_output.txt"
