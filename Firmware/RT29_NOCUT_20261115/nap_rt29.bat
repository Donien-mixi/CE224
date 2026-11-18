@echo off
setlocal EnableDelayedExpansion
title NAP RT29-NOCUT / NET30
set "REL=%~dp0"
set "CP=C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
set "ES=C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
echo ============================================================
echo  NAP RT29-NOCUT / NET30  (ban du phong: 50/72/80, khong bu banh trai)
echo  Nap CA HAI board. Ban cu: ba muc 50/72/80, chua co bu banh trai.
echo ============================================================
echo.
if not exist "%CP%" ( echo [LOI] Khong thay STM32_Programmer_CLI.exe & goto :fail )
if not exist "%ES%" ( echo [LOI] Khong thay esptool.exe & goto :fail )
if not exist "%REL%STM32_BAL_R12_RT29_NOCUT.hex" ( echo [LOI] Thieu file hex & goto :fail )
set /p COM=So COM cua ESP32 (vi du COM3, bo trong neu chi nap STM32):
echo.
echo --- Nap STM32 ---
"%CP%" -c port=SWD -w "%REL%STM32_BAL_R12_RT29_NOCUT.hex" -v -rst
if errorlevel 1 ( echo [LOI] Nap STM32 that bai & goto :fail )
if "%COM%"=="" goto :done
echo.
echo --- Nap ESP32 tren %COM% ---
"%ES%" --chip esp32s3 --port %COM% --baud 921600 write-flash ^
  0x0 "%REL%ESP32_RT29_NOCUT_NET30.bootloader.bin" ^
  0x8000 "%REL%ESP32_RT29_NOCUT_NET30.partitions.bin" ^
  0xe000 "%REL%ESP32_RT29_NOCUT_NET30.boot_app0.bin" ^
  0x10000 "%REL%ESP32_RT29_NOCUT_NET30.app.bin"
if errorlevel 1 ( echo [LOI] Nap ESP32 that bai & goto :fail )
:done
echo.
echo ============================================================
echo  XONG. Banner: RT29-NOCUT-NET30 ; nut: 50 / 72 / 80
echo ============================================================
pause
exit /b 0
:fail
echo.
echo  Khong nap duoc. Kiem tra day cam, nguon, driver ST-LINK/CP210x, so COM.
pause
exit /b 1