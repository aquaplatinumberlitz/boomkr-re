@echo off
rem usage: kr_dump2.bat <OUTDIR> [EXE_NAME]
set OUT=%1
set EXE=%2
if "%EXE%"=="" set EXE=CA.exe
\\host.lan\Data\boomkr-full\krdump32.exe %EXE% %OUT%
type %OUT%\dump.log
