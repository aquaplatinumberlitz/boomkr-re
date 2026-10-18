@echo off
rem Hook RtlExitUserProcess + chup process tree ngay khi client chay
taskkill /F /IM CA.exe >nul 2>&1
start "" V:\boomkr-full\krhookpro.exe CA.exe C:\krdumpR1 30 ntdll.dll RtlExitUserProcess 5
timeout /t 2 >nul
start "" V:\boomkr-full\CA.exe
timeout /t 3 >nul
wmic process where "name='CA.exe'" get ProcessId,ParentProcessId,ThreadCount,WorkingSetSize > V:\boomkr-re\out\ptree.txt
timeout /t 26 >nul
copy /Y C:\krdumpR1\hookpro.log V:\boomkr-re\out\hookproR1.log >nul
echo DONE
