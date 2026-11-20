@echo off
setlocal EnableDelayedExpansion
title NAP RT41-LINK / NET41
set "REL=%~dp0"
set "CP=C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
set "ES=C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
echo ============================================================
echo  NAP RT41-LINK / NET41  (UART 921600 + chon kenh Wi-Fi)
echo  Nap CA HAI board: doi UART nen phai nap ca STM32 lan ESP32.
echo ============================================================
echo.
if not exist "%CP%" ( echo [LOI] Khong thay STM32_Programmer_CLI.exe & goto :fail )
if not exist "%ES%" ( echo [LOI] Khong thay esptool.exe & goto :fail )
if not exist "%REL%STM32_BAL_R12_RT41_LINK.hex" ( echo [LOI] Thieu file hex & goto :fail )
if not exist "%REL%ESP32_RT41_LINK_NET41.app.bin" ( echo [LOI] Thieu file app.bin & goto :fail )
set /p COM=So COM cua ESP32 (vi du COM3, BAT BUOC cho ban nay):
if "%COM%"=="" ( echo [LOI] Ban RT41 doi UART nen phai nap ESP32 cung luc. & goto :fail )
echo.
echo --- Nap STM32 ---
"%CP%" -c port=SWD -w "%REL%STM32_BAL_R12_RT41_LINK.hex" -v -rst
if errorlevel 1 ( echo [LOI] Nap STM32 that bai & goto :fail )
echo.
echo --- Nap ESP32 tren %COM% ---
"%ES%" --chip esp32s3 --port %COM% --baud 921600 write-flash ^
  0x0 "%REL%ESP32_RT41_LINK_NET41.bootloader.bin" ^
  0x8000 "%REL%ESP32_RT41_LINK_NET41.partitions.bin" ^
  0xe000 "%REL%ESP32_RT41_LINK_NET41.boot_app0.bin" ^
  0x10000 "%REL%ESP32_RT41_LINK_NET41.app.bin"
if errorlevel 1 ( echo [LOI] Nap ESP32 that bai & goto :fail )
:done
echo.
echo ============================================================
echo  XONG. Banner: RT41-LINK-NET41 ; nut: 50 / 80 / MAX 90
echo  Kiem tra Serial: dong [NET41] scan=... chN ; va log phai co crc_errors=0.
echo ============================================================
pause
exit /b 0
:fail
echo.
echo  Khong nap duoc. Kiem tra day cam, nguon, driver ST-LINK/CP210x, so COM.
pause
exit /b 1
