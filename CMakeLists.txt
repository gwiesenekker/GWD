cmake_minimum_required(VERSION 3.18)
project(gwd8 LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake -S <source> -B <writable-build-directory>")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# Match the /MD runtime used by the Windows dependencies, including Debug.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library")
set(COMPAT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../compat" CACHE PATH "Shared compat sources")
set(ENDGAME7_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../endgame7" CACHE PATH "endgame7 sources")
set(GWD_DEPENDENCY_DIR "" CACHE PATH "Directory containing SQLite3/Zstd headers and libraries")
option(GWD_USE_OPENMPI "Enable OpenMPI (Linux only)" OFF)
option(GWD_USE_AVX2 "Enable GWD evaluation intrinsics on x86-64 (xxHash remains AVX2)" ON)
option(BUILD_TESTING "Build/register tests" OFF)

if(WIN32)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Windows builds require Clang or clang-cl, as in endgame7.")
    endif()
    if(GWD_USE_OPENMPI)
        message(FATAL_ERROR "OpenMPI is not supported by this Windows build.")
    endif()
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "GWD8 requires a 64-bit build.")
endif()
foreach(path "${COMPAT_DIR}/compat.c" "${ENDGAME7_DIR}/CMakeLists.txt")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Missing dependency source: ${path}")
    endif()
endforeach()
find_path(ZSTD_INCLUDE_DIR zstd.h HINTS "${GWD_DEPENDENCY_DIR}" PATH_SUFFIXES include REQUIRED)
find_library(ZSTD_LIBRARY NAMES zstd libzstd zstd_static libzstd_static
    HINTS "${GWD_DEPENDENCY_DIR}" PATH_SUFFIXES lib REQUIRED)
find_path(SQLITE3_INCLUDE_DIR sqlite3.h HINTS "${GWD_DEPENDENCY_DIR}" PATH_SUFFIXES include REQUIRED)
find_library(SQLITE3_LIBRARY NAMES sqlite3 sqlite3_static
    HINTS "${GWD_DEPENDENCY_DIR}" PATH_SUFFIXES lib REQUIRED)

# endgame7 owns compat.c; do not add it to the executable a second time.
# All subproject outputs go to the writable build tree.
add_subdirectory("${ENDGAME7_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/endgame7" EXCLUDE_FROM_ALL)

add_executable(gwd8
    main.c bstrlib.c cJSON.c xxhash.c my_safec.c boards.c book.c
    buckets.c caches.c classes.c compress.c dxp.c endgame.c fbuffer.c
    hub.c mcts.c moves.c my_bstreams.c my_cjson.c my_malloc.c my_mpi.c
    my_printf.c my_random.c my_sqlite3.c networks.c patterns.c profile.c
    pdn.c queues.c records.c score.c search.c states.c stats.c
    my_threads.c timers.c utils.c)
target_include_directories(gwd8 PRIVATE "${COMPAT_DIR}" "${ENDGAME7_DIR}"
    "${ZSTD_INCLUDE_DIR}" "${SQLITE3_INCLUDE_DIR}")
target_compile_definitions(gwd8 PRIVATE USE_SQLITE3 $<$<CONFIG:Debug>:DEBUG>)
target_link_libraries(gwd8 PRIVATE gwdegtb "${SQLITE3_LIBRARY}")

if(WIN32)
    target_compile_definitions(gwd8 PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_definitions(gwd8 PRIVATE _GNU_SOURCE)
    target_link_libraries(gwd8 PRIVATE m)
endif()
if(MSVC)
    target_compile_options(gwd8 PRIVATE /W3)
else()
    target_compile_options(gwd8 PRIVATE -Wall -Wshadow)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|X86_64)$")
    # Keep xxHash and the compiler target unchanged when comparing evaluators.
    target_compile_definitions(gwd8 PRIVATE XXH_VECTOR=XXH_AVX2)
    if(GWD_USE_AVX2)
        target_compile_definitions(gwd8 PRIVATE USE_AVX2_INTRINSICS)
    endif()
    if(MSVC)
        target_compile_options(gwd8 PRIVATE /arch:AVX2 /clang:-mbmi2 /clang:-mfma)
    else()
        target_compile_options(gwd8 PRIVATE -mavx2 -msse4.2 -mbmi2 -mfma)
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$" AND NOT WIN32)
    target_compile_options(gwd8 PRIVATE -march=armv8-a+simd+crc)
else()
    message(FATAL_ERROR "Supported targets: x86-64 (AVX2), or Linux AArch64 with CRC.")
endif()
if(GWD_USE_OPENMPI)
    find_package(MPI REQUIRED COMPONENTS C)
    target_compile_definitions(gwd8 PRIVATE USE_OPENMPI)
    target_link_libraries(gwd8 PRIVATE MPI::MPI_C)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME gwd8_revision COMMAND gwd8 --revision)
endif()
