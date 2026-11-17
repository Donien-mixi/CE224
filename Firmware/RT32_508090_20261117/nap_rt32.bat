@echo off
setlocal EnableDelayedExpansion
title NAP RT32-508090 / NET33  (50 / 80 / MAX 90 cm/s)
set "REL=%~dp0"
set "CP=C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
set "ES=C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"

echo ============================================================
echo  NAP FIRMWARE CHOT  RT32-508090 / NET33
echo  Ba muc toc do: 50 / 80 / MAX 90 cm/s  (+ bu lech banh trai)
echo  Luu y: MAX 90 chi dung khi pin do duoc ^>= 12,0 V
echo ============================================================
echo.
if not exist "%CP%" ( echo [LOI] Khong thay STM32_Programmer_CLI.exe & goto :fail )
if not exist "%ES%" ( echo [LOI] Khong thay esptool.exe & goto :fail )
if not exist "%REL%STM32_BAL_R12_RT32_508090.hex" ( echo [LOI] Thieu file hex trong thu muc nay & goto :fail )

set /p COM=So COM cua ESP32 (vi du COM3, bo trong neu chi nap STM32):
echo.
echo --- Nap STM32 ---
"%CP%" -c port=SWD -w "%REL%STM32_BAL_R12_RT32_508090.hex" -v -rst
if errorlevel 1 ( echo [LOI] Nap STM32 that bai & goto :fail )

if "%COM%"=="" goto :done
echo.
echo --- Nap ESP32 tren %COM% ---
"%ES%" --chip esp32s3 --port %COM% --baud 921600 write-flash ^
  0x0 "%REL%ESP32_RT32_508090_NET33.bootloader.bin" ^
  0x8000 "%REL%ESP32_RT32_508090_NET33.partitions.bin" ^
  0xe000 "%REL%ESP32_RT32_508090_NET33.boot_app0.bin" ^
  0x10000 "%REL%ESP32_RT32_508090_NET33.app.bin"
if errorlevel 1 ( echo [LOI] Nap ESP32 that bai & goto :fail )

:done
echo.
echo ============================================================
echo  XONG. Mo http://192.168.4.1/ui/r12
echo  Banner phai la RT32-508090-NET33, ba nut: 50 / 80 / MAX 90.
echo ============================================================
pause
exit /b 0

:fail
echo.
echo  Khong nap duoc. Kiem tra day cam, nguon board, driver ST-LINK/CP210x, so COM.
pause
exit /b 1
