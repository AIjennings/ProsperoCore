@echo off
title Prospero PS5 Emulator - Launching Game...
cd /d "%~dp0"
echo =======================================================
echo     PlayStation 5 (Prospero) Next-Gen Emulator
echo =======================================================
echo.
echo Launching: Prospero: Void Runner (PS5 Showcase)
echo Title ID:  PPSA09999
echo Target:    60 FPS / Direct GDDR6 / AMD RDNA2 PM4
echo Controls:  [WASD / Arrows] Fly  [Space / Cross] Fire  [Shift / R2] Boost
echo.
.\build\Release\prospero_cli.exe run games/void_runner
pause
