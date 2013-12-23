@echo off
reg add "HKLM\Software\Wow6432Node\Google\Chrome\Extensions\ikoecccpabgkgdjemjjfkcmpjhchnalh" /f /v path /t REG_SZ /d "%~dp0chrome.crx"
reg add "HKLM\Software\Wow6432Node\Google\Chrome\Extensions\ikoecccpabgkgdjemjjfkcmpjhchnalh" /f /v version /t REG_SZ /d "1.0"
