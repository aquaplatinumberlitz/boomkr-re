@echo off
rem usage: kr_dump.bat <PID> [OUTDIR]  -- dumper 32-bit
set PID=%1
set OUT=%2
if "%OUT%"=="" set OUT=C:\krdumpX
\\host.lan\Data\boomkr-full\krdump32.exe %PID% %OUT%
type %OUT%\dump.log
