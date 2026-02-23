# Generate the Windows event template files

1. Open Visual Studio Developer Powershell

2. Generate message template dll:

```powershell
cd resources/windows
mc -c .\EventLog.mc
rc .\EventLog.rc
link -dll -noentry .\EventLog.res
move .\EventLog.h ..\..\src\platform\windows\
```
