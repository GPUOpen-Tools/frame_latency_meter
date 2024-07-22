set CurrDir=%CD%
for %%* in (.) do set CurrDirName=%%~nx*

del  /s *.sdf
del  /s *.db
del  /s *.pdb
del  /s *.idb
del  /s *.ipch
del  /s *.user
del  /s *.opendb
del  /s *.orig
del  /s CMakeCache.txt
del  /s cmake_install.cmake
del  /s *.a
del  /s *.o
del  Makefile
rmdir /s  /q bin
rmdir /s  /q lib

IF EXIST win (
rmdir  /s /q -r win
)

cd %CurrDir%

FOR /d /r . %%d IN (CMakeFiles) DO @IF EXIST "%%d" rd /s /q "%%d"
FOR /d /r . %%d IN (*_autogen) DO @IF EXIST "%%d" rd /s /q "%%d"

