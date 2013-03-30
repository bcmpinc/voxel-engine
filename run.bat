@echo off
SET PATH=C:\mingw64\bin;C:\mingw64\x86_64-w64-mingw32\bin;C:\Program Files (x86)\CMake 2.8\bin;%PATH%
make -j 8 || goto fail

vox

goto end

:fail
pause

:end