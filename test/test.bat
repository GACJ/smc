@echo off

pushd %~dp0

set SMC32=..\bin.release\smc32.exe
set PLATFORM=Win32
if "%1" == "x64" (
    set SMC32=..\bin.release\smc64.exe
    set PLATFORM=x64
)
echo Running for %PLATFORM%

set FINALRESULT=0

REM Clean output files
del data\*.sf1 data\*.cf1

REM Test case 1:
echo ---------------------------------------------------------------
echo TEST CASE 1
echo ---------------------------------------------------------------
%SMC32% data\ca01.smc
%SMC32% --deterministic-output data\ca01.sf1 data\music8.mus
fc data\ca01.cf1 data\ca01.cf1.expected
echo ---------------------------------------------------------------
if errorlevel 1 (
    echo [31mFAILED[0m
    set FINALRESULT=1
) else (
    echo [32mPASSED[0m
)
echo ---------------------------------------------------------------

REM Test case 2:
echo ---------------------------------------------------------------
echo TEST CASE 2
echo ---------------------------------------------------------------
%SMC32% --deterministic-output data\ca10a.smc
fc data\ca10a.sf1 data\ca10a.sf1.expected
echo ---------------------------------------------------------------
if errorlevel 1 (
    echo [31mFAILED[0m
    set FINALRESULT=1
) else (
    echo [32mPASSED[0m
)
echo ---------------------------------------------------------------

REM Test case 3:
echo ---------------------------------------------------------------
echo TEST CASE 3
echo ---------------------------------------------------------------
%SMC32% --deterministic-output data\ca08jf.smc
fc data\ca08jf.sf1 data\ca08jf.sf1.expected
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
