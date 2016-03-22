@echo off

if not exist ".\extern\build" (
  mkdir .\extern\build
  cd .\extern\build

  cmake ..

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
cmake --build . --config Release

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