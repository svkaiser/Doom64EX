@echo off

if not exist ".\extern\build" (
  mkdir .\extern\build
  cd .\extern\build

  cmake -G "MinGW Makefiles" ..

  if %ERRORLEVEL% neq 0 (
    echo.
    echo Configure error!
    echo Make sure cmake is in your PATH
    pause
    exit
  )

  echo -
  echo Done configuring!
  echo Verify that everything is in order before you continue.
  pause
) else (
  echo.
  echo Using the existing build directory...
  echo.
)

cd .\extern\build
mingw32-make

if %ERRORLEVEL% neq 0 (
  echo.
  echo Build error!
  pause
  exit
)

echo.
echo DONE building!!!
echo Congrats!
pause