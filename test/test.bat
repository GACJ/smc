@echo off

pushd %~dp0

set FINALRESULT=0
set SMC32=..\bin\Win32\Release\smc32.exe

REM Clean output files
del data\*.sf1 data\*.cf1

REM Test case 1:
echo ---------------------------------------------------------------
echo TEST CASE 1
echo ---------------------------------------------------------------
%SMC32% data\ca01.smc
%SMC32% data\ca01.sf1 data\music8.mus
fc data\ca01.cf1 data\ca01.cf1.expected
echo ---------------------------------------------------------------
if errorlevel 1 (
    echo [31mFAILED[0m
    set FINALRESULT=1
) else (
    echo [32mPASSED[0m
)
echo ---------------------------------------------------------------

popd
exit /b %FINALRESULT%