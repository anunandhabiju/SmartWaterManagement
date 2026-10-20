@echo off
setlocal

echo =================================================================
echo  Launching Smart Water Distribution and Management System
echo =================================================================
echo.

if not exist "backend\server.exe" (
    echo Compiling application first...
    call build.bat
    if errorlevel 1 (
        echo [ERROR] Build failed! Cannot start application.
        pause
        exit /b 1
    )
)

:: Check WinGet LLVM-MinGW directory if runtime DLLs are needed
set "WINGET_LLVM=%LOCALAPPDATA%\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
if exist "%WINGET_LLVM%" (
    set "PATH=%WINGET_LLVM%;%PATH%"
)

echo Starting C Backend Server on http://localhost:8080 ...
echo Opening web browser...
start http://localhost:8080/index.html

:: Run server directly in this console window so user sees real-time log outputs
cd backend
server.exe 8080
cd ..
