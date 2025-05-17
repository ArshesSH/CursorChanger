@echo off
echo Remove CursorChanger from startup
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run" /v "CursorChanger" /f
if %errorlevel% == 0 (
  echo Success: CursorChanger has been removed from startup.
) else (
  echo Note: CursorChanger was not found in startup.
)


set "SettingsFile=settings.ini"
if exist "%SettingsFile%" (
  echo %SettingsFile% Edit setting.ini
  
  powershell -Command "(Get-Content '%SettingsFile%') | ForEach-Object { $_ -replace 'ShouldRegisterStartUp=1', 'ShouldRegisterStartUp=0' } | Set-Content '%SettingsFile%'"
  
  echo Success: %SettingsFile% has been updated.
) else (
  echo Note: %SettingsFile% not found.
)
pause