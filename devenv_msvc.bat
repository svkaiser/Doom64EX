@echo off

if exist ".\devenv\" (
  echo.
  echo The devenv directory already exists!
  echo If you continue, it'll be regenerated.
  echo.
  pause

)

mkdir .\devenv
cd .\devenv

cmake ..

if %ERRORLEVEL% neq 0 (
  echo.
  echo Configure error!
  echo Make sure cmake is in your PATH
  pause
  exit
)

echo.
echo Done configuring!
echo Open the .sln inside devenv with Visual Studio
pause