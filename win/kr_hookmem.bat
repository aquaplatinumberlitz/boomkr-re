@echo off
rem usage: kr_hookmem.bat [secs]
set S=%1
if "%S%"=="" set S=60
start "" C:\boomkr\CA.exe
timeout /t 5 /nobreak >nul
\\host.lan\Data\boomkr-full\krhookmem.exe CA.exe C:\krdumpH %S%
type C:\krdumpH\hookmem.log
