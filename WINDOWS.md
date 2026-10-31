# GWD8 on Windows 11 (64-bit Clang)

This build enables SQLite3 and links endgame7. OpenMPI, Valgrind and the
Linux profiling instrumentation are disabled. Search threads remain supported.
The x86-64 executable requires AVX2, SSE4.2, BMI2 and FMA in the VM.

`GWD_USE_AVX2` defaults to `ON`. Setting it to `OFF` disables only GWD's
handwritten evaluation intrinsics (`USE_AVX2_INTRINSICS`). xxHash still uses
AVX2, and the AVX2/FMA compiler flags remain enabled in both modes. This is
an evaluator comparison switch, not support for CPUs without AVX2.

To switch an already configured Release build, run:

```bat
cmake -S G:/ -B C:/Tmp2/gwd8/build-release -DGWD_USE_AVX2=OFF
cmake --build C:/Tmp2/gwd8/build-release --target gwd8 --parallel 8
```

Use `-DGWD_USE_AVX2=ON` and rebuild to restore the evaluation intrinsics.
SQLite3 remains enabled in both modes.

## Directory layout

| Purpose | Directory |
| --- | --- |
| endgame7 sources (read-only) | `E:\` |
| GWD8 sources (read-only) | `G:\` |
| shared compat sources (read-only) | `F:\` |
| dependency headers/libraries and writable output | `C:\Tmp2\gwd8` |

The drive roots must contain the source files themselves, including
`E:\CMakeLists.txt`, `G:\CMakeLists.txt` and `F:\compat.c`.
Adjust the source paths if they contain another directory level.

Use the same Visual Studio x64 developer environment, CMake, Ninja and
`clang-cl` installation that built endgame7. Run the commands below in a
Visual Studio x64 Developer Command Prompt with the x64 compiler/linker paths.

## Dependencies

Place `zstd.h` and `sqlite3.h` in `C:\Tmp2\gwd8\include`. You also need
**64-bit Windows libraries**, not just headers or DLLs:

- The Zstd library at `C:\Tmp2\gwd8\lib\libzstd.lib`.
- A SQLite3 import library or compatible static library at
  `C:\Tmp2\gwd8\lib\sqlite3.lib`.

Use matching MSVC ABI dependencies; this build defaults to the DLL CRT
(`/MD`), including Debug. The explicit library filenames below are examples:
replace them with your actual filenames.

If SQLite provides only `sqlite3.dll` and `sqlite3.def`, create its import
library from the x64 developer terminal:

```bat
lib /def:C:\Tmp2\gwd8\sqlite3.def /machine:x64 /out:C:\Tmp2\gwd8\lib\sqlite3.lib
```

## Configure and build

If a build directory was configured with the old drive mappings, add
`--fresh` to its first configure command below (`cmake --fresh -S G:/ ...`).
This clears CMake's cached configuration without deleting runtime files.
Use separate build directories for Debug and Release.

Debug:

```bat
cmake -S G:/ -B C:/Tmp2/gwd8/build-debug -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Debug -DCOMPAT_DIR=F:/ -DENDGAME7_DIR=E:/ -DZSTD_INCLUDE_DIR=C:/Tmp2/gwd8/include -DZSTD_LIBRARY=C:/Tmp2/gwd8/lib/libzstd.lib -DSQLITE3_INCLUDE_DIR=C:/Tmp2/gwd8/include -DSQLITE3_LIBRARY=C:/Tmp2/gwd8/lib/sqlite3.lib -DGWD_USE_OPENMPI=OFF -DBUILD_TESTING=ON
cmake --build C:/Tmp2/gwd8/build-debug --parallel 8
ctest --test-dir C:/Tmp2/gwd8/build-debug --output-on-failure
```

Release:

```bat
cmake -S G:/ -B C:/Tmp2/gwd8/build-release -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release -DCOMPAT_DIR=F:/ -DENDGAME7_DIR=E:/ -DZSTD_INCLUDE_DIR=C:/Tmp2/gwd8/include -DZSTD_LIBRARY=C:/Tmp2/gwd8/lib/libzstd.lib -DSQLITE3_INCLUDE_DIR=C:/Tmp2/gwd8/include -DSQLITE3_LIBRARY=C:/Tmp2/gwd8/lib/sqlite3.lib -DGWD_USE_OPENMPI=OFF -DBUILD_TESTING=ON
cmake --build C:/Tmp2/gwd8/build-release --parallel 8
ctest --test-dir C:/Tmp2/gwd8/build-release --output-on-failure
```

The executable is `C:\Tmp2\gwd8\build-debug\gwd8.exe`. Nothing is generated
in E:, F: or G:. The endgame7 build and its copy of compat are compiled under
the build directory. There is no automatic deployment or source-file copying.

The optimized executable is `C:\Tmp2\gwd8\build-release\gwd8.exe`.
Use separate build directories for different compilers/generators.

## Run

For DLL dependencies, put the DLLs in the executable directory or add their
directory to PATH. For example, if they are in `C:\Tmp2\gwd8`:

```bat
set "PATH=C:\Tmp2\gwd8;%PATH%"
C:\Tmp2\gwd8\build-debug\gwd8.exe --revision
ctest --test-dir C:/Tmp2/gwd8/build-debug -R "^gwd8_revision$" --output-on-failure
```

The revision check needs no engine configuration. For search, use a **writable
working directory**: GWD creates logs, temporary FEN files and other outputs.

```bat
if not exist C:\Tmp2\gwd8\run mkdir C:\Tmp2\gwd8\run
copy G:\gwd.json C:\Tmp2\gwd8\run\
copy G:\overrides.json C:\Tmp2\gwd8\run\
copy G:\network0.json C:\Tmp2\gwd8\run\
copy G:\std.fen C:\Tmp2\gwd8\run\
cd /d C:\Tmp2\gwd8\run
```

Review the copied configuration: change Linux paths to your Windows paths,
especially network, book and EGTB directories. Ensure the selected overrides
do not restore Linux paths. Use only databases that actually exist.
Do not edit the read-only source copies.

Then, with the configuration set up:

```bat
C:\Tmp2\gwd8\build-debug\gwd8.exe -d=8 -t=5 solve_problems std.fen
```

For actual multithreaded testing, use `solve_problems_multi_threaded` with a
file containing plain FEN lines (no score annotations or trailing `*`):

```bat
C:\Tmp2\gwd8\build-debug\gwd8.exe --nthreads_alpha_beta=4 -d=8 -t=5 solve_problems_multi_threaded plain.fen
```

In this baseline, `solve_problems` itself is single-threaded even if the thread
count option is set. No MPI launch is needed for either command.

## Linux CMake cross-check

```sh
cmake -S . -B /tmp/gwd8-build -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/gwd8-build --target gwd8 -j4
```

The existing Makefile is unchanged. CMake defaults to no MPI on Linux too;
use `-DGWD_USE_OPENMPI=ON` to enable it there.
