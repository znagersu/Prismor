@echo off
setlocal
cd /d "%~dp0\.."
echo Running Prismor Automated Release Pipeline...
node scripts\publish_release.js
if %ERRORLEVEL% neq 0 (
    echo Release failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
echo Done.
