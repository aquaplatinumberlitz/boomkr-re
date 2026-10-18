@echo off
echo === System locale hien tai:
powershell -NoProfile -Command "Get-WinSystemLocale | Select-Object -ExpandProperty Name"
echo === Doi sang ko-KR:
powershell -NoProfile -Command "Set-WinSystemLocale -SystemLocale ko-KR; Get-WinSystemLocale | Select-Object -ExpandProperty Name"
echo === Xong (can reboot)
