@echo off
SET PATH=C:\mingw64\bin;C:\mingw64\x86_64-w64-mingw32\bin;C:\Program Files (x86)\CMake 2.8\bin;%PATH%

:ask
CHOICE /C VCHQ /M "[V]ox, [C]onvert, [H]eightmap [Q]uit"
IF ERRORLEVEL 4 GOTO quit:
IF ERRORLEVEL 3 GOTO heightmap:
IF ERRORLEVEL 2 GOTO convert:
IF ERRORLEVEL 1 GOTO vox:
GOTO ask

:vox
make -j 8 vox || goto ask
vox
goto ask

:convert
make -j 8 convert || goto ask
if not exist points.vxl (
  convert points
) else (
  echo File already exists.
)
goto ask

:heightmap
make -j 8 heightmap || goto ask
heightmap
goto ask

:quit