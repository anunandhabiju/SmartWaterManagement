@echo off
setlocal enabledelayedexpansion

echo =================================================================
echo  Smart Water Distribution and Management System - Build Script
echo  UN SDG 6: Clean Water and Sanitation Data Structures Project
echo =================================================================
echo.

:: Detect Compiler
set "CC="

where clang >nul 2>nul
if %errorlevel% equ 0 (
    set "CC=clang"
    goto COMPILER_FOUND
)

where gcc >nul 2>nul
if %errorlevel% equ 0 (
    set "CC=gcc"
    goto COMPILER_FOUND
)

:: Check WinGet LLVM-MinGW directory
set "WINGET_LLVM=%LOCALAPPDATA%\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
if exist "%WINGET_LLVM%\clang.exe" (
    set "PATH=%WINGET_LLVM%;%PATH%"
    set "CC=clang"
    goto COMPILER_FOUND
)

:: Check standard MinGW locations
if exist "C:\mingw64\bin\gcc.exe" (
    set "PATH=C:\mingw64\bin;%PATH%"
    set "CC=gcc"
    goto COMPILER_FOUND
)

if exist "C:\msys64\mingw64\bin\gcc.exe" (
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
    set "CC=gcc"
    goto COMPILER_FOUND
)

echo [ERROR] No suitable C compiler (clang or gcc) found on system!
echo Please install MinGW or LLVM, or ensure gcc/clang is in your PATH.
exit /b 1

:COMPILER_FOUND
echo [OK] Using C Compiler: %CC%
echo.

:: Create Directories
if not exist "backend\data" mkdir "backend\data"
if not exist "tests" mkdir "tests"

:: Compile Automated Unit Test Suite
echo [1/3] Compiling Data Structures Unit Test Suite (tests/test_all.c)...
%CC% -Wall -Wextra tests/test_all.c backend/tank.c backend/queue.c backend/linkedlist.c backend/bst.c backend/graph.c backend/distribution.c backend/storage.c -o tests/test_all.exe
if %errorlevel% neq 0 (
    echo [ERROR] Unit test compilation failed!
    exit /b 1
)
echo [OK] Unit test executable compiled: tests/test_all.exe
echo.

:: Execute Unit Tests
echo [2/3] Running Automated Unit Tests for all 5 Data Structures...
tests\test_all.exe
if %errorlevel% neq 0 (
    echo [ERROR] One or more unit tests failed!
    exit /b 1
)
echo [OK] All Data Structure Unit Tests Passed!
echo.

:: Compile Backend HTTP Server
echo [3/3] Compiling Mongoose C REST Server (backend/server.exe)...
%CC% -Wall -Wextra -Wno-unused-function -Wno-unused-parameter backend/main.c backend/server.c backend/tank.c backend/queue.c backend/linkedlist.c backend/bst.c backend/graph.c backend/distribution.c backend/storage.c backend/cJSON.c backend/mongoose.c -lws2_32 -o backend/server.exe
if %errorlevel% neq 0 (
    echo [ERROR] Backend server compilation failed!
    exit /b 1
)
echo [OK] Backend server binary created: backend/server.exe
echo.

echo =================================================================
echo  BUILD SUCCESSFUL!
echo  Run "run.bat" to start the server and launch the web dashboard.
echo =================================================================
