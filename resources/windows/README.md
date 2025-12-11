# Generate the Windows event template files

1. Open Visual Studio Developer Powershell

2. Generate message template dll:

```powershell
cd resources/windows
mc -c .\Event_log.mc
rc .\Event_log.rc
link -dll -noentry .\Event_log.res
move .\Event_log.h ..\..\src\platform\windows\
```
