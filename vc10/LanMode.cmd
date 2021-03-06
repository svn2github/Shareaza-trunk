@echo off
setlocal

cd ..
for /F "tokens=3" %%a in ( 'findstr ProductVersion shareaza\Shareaza.rc' ) do set VERSION=%%~a

set PLATFORM=Win32
set CONFIG=Release
set COMPILER=vc10
set BUILDDIR=%COMPILER%\%PLATFORM%\%CONFIG%
set BUILDPRP=%COMPILER%\LanMode.props
set BUILDTMP=%BUILDDIR%\LanMode.tmp.cmd
set BUILDRUN=%BUILDDIR%\LanMode.run.cmd
set BUILDLNK=..\Releases\%VERSION%-LAN\
set BUILDOUT=..\Releases\Shareaza-%VERSION%-LAN\

echo Builing Shareaza %VERSION% %PLATFORM% %CONFIG%...

set SubWCRev="%ProgramFiles%\TortoiseSVN\bin\SubWCRev.exe"
if exist %SubWCRev% goto ok
set SubWCRev="%ProgramFiles(x86)%\TortoiseSVN\bin\SubWCRev.exe"
if exist %SubWCRev% goto ok
set SubWCRev="%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe"
if exist %SubWCRev% goto ok
echo The SubWCRev utility is missing. Please go to https://sourceforge.net/projects/tortoisesvn/ and install TortoiseSVN.
goto errors
:ok
echo Using TortoiseSVN at %SubWCRev%...

echo Cleaning...
call clean.cmd
md %BUILDDIR% 2>nul
del /q %BUILDTMP% 2>nul
del /q %BUILDRUN% 2>nul
del /q setup\builds\Preprocessed.iss 2>nul

echo Creating build script...
echo setlocal > %BUILDTMP%
echo rmdir %BUILDLNK% >> %BUILDTMP%
echo mklink /D /J %BUILDLNK% . >> %BUILDTMP%
echo pushd %BUILDLNK% >> %BUILDTMP%
echo call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" x86 >> %BUILDTMP%
echo set ISCCOPTIONS=/dRELEASE_BUILD=1 /dLAN_MODE /q >> %BUILDTMP%
echo msbuild %COMPILER%\Shareaza.sln /nologo /v:m /t:Rebuild /p:ForceImportBeforeCppTargets="%CD%\%BUILDPRP%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /fl /flp:Summary;Verbosity=normal;LogFile=%BUILDDIR%\Shareaza_r$WCREV$_$WCNOW=%%Y-%%m-%%d$_LAN.log >> %BUILDTMP%
echo popd >> %BUILDTMP%
echo rmdir %BUILDLNK% >> %BUILDTMP%
echo md %BUILDOUT% >> %BUILDTMP%
echo move /y setup\builds\Shareaza_%VERSION%_%PLATFORM%.exe %BUILDOUT%Shareaza_%VERSION%_%PLATFORM%_LAN.exe >> %BUILDTMP%
echo if errorlevel 1 exit /b 1 >> %BUILDTMP%
echo move /y setup\builds\Shareaza_%VERSION%_%PLATFORM%_Symbols.7z %BUILDOUT%Shareaza_%VERSION%_%PLATFORM%_LAN_Symbols.7z >> %BUILDTMP%
echo if errorlevel 1 exit /b 1 >> %BUILDTMP%
echo move /y setup\builds\Shareaza_%VERSION%_Source.7z %BUILDOUT% >> %BUILDTMP%
echo if errorlevel 1 exit /b 1 >> %BUILDTMP%
%SubWCRev% . %BUILDTMP% %BUILDRUN%
del /q %BUILDTMP% 2>nul

echo Building...
call %BUILDRUN%
if errorlevel 1 goto errors
del /q %BUILDRUN% 2>nul
exit /b 0

:errors
echo Build failed!
pause