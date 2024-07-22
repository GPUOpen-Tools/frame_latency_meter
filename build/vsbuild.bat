cd ..\
set CurrDir=%CD%
for %%* in (.) do set CurrDirName=%%~nx*
IF EXIST %CurrDir%\build\win (rmdir build\win /s /q)
mkdir build\win 

cd build\win
cmake -G "Visual Studio 17 2022"  ..\..\..\%CurrDirName%
cd %CurrDir%
