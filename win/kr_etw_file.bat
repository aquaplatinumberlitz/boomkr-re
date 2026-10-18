@echo off
logman stop KF -ets >nul 2>&1
del C:\kf.etl C:\kf.csv >nul 2>&1
echo === bat trace
logman create trace KF -p Microsoft-Windows-Kernel-File 0xFFFF -o C:\kf.etl -ets
timeout /t 2 >nul
taskkill /F /IM CA.exe >nul 2>&1
echo === start client
start "" V:\boomkr-full\CA.exe
timeout /t 12 >nul
echo === stop trace
logman stop KF -ets
echo === convert
tracerpt C:\kf.etl -o V:\boomkr-re\out\kf.csv -of CSV -y
echo DONE
