@echo off
setlocal
rem Run from a Visual Studio x64 Developer Command Prompt.
rem Sources: GWD8=G:, compat=F:, endgame7=E:.

cmake --fresh -S G:/ -B C:/Tmp2/gwd8/build-release -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release -DCOMPAT_DIR=F:/ -DENDGAME7_DIR=E:/ -DZSTD_INCLUDE_DIR=C:/Tmp2/gwd8/include -DZSTD_LIBRARY=C:/Tmp2/gwd8/lib/libzstd.lib -DSQLITE3_INCLUDE_DIR=C:/Tmp2/gwd8/include -DSQLITE3_LIBRARY=C:/Tmp2/gwd8/lib/sqlite3.lib -DGWD_USE_AVX2=ON -DGWD_USE_OPENMPI=OFF -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1

cmake --build C:/Tmp2/gwd8/build-release --target gwd8 --clean-first --parallel 8
if errorlevel 1 exit /b 1

"C:\Tmp2\gwd8\build-release\gwd8.exe" --revision
if errorlevel 1 exit /b 1

dumpbin /DEPENDENTS C:\Tmp2\gwd8\build-release\gwd8.exe
if errorlevel 1 exit /b 1

endlocal
exit /b 0
