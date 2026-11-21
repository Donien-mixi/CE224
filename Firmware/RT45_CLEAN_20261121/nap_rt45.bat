@echo off
setlocal EnableDelayedExpansion
title NAP RT45-CLEAN / NET45
set "REL=%~dp0"
set "CP=C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
set "ES=C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
echo ============================================================
echo  NAP RT45-CLEAN / NET45  (ban don code, khong doi hanh vi)
echo  Nap CA HAI board: build ID doi o ca hai dau, ESP32 kiem tra BINFO.
echo ============================================================
echo.
if not exist "%CP%" ( echo [LOI] Khong thay STM32_Programmer_CLI.exe & goto :fail )
if not exist "%ES%" ( echo [LOI] Khong thay esptool.exe & goto :fail )
if not exist "%REL%STM32_BAL_R12_RT45_CLEAN.hex" ( echo [LOI] Thieu file hex & goto :fail )
if not exist "%REL%ESP32_RT45_CLEAN_NET45.app.bin" ( echo [LOI] Thieu file app.bin & goto :fail )
set /p COM=So COM cua ESP32 (vi du COM3, BAT BUOC):
if "%COM%"=="" ( echo [LOI] Ban nay phai nap ca ESP32. & goto :fail )
echo.
echo --- Nap STM32 ---
"%CP%" -c port=SWD -w "%REL%STM32_BAL_R12_RT45_CLEAN.hex" -v -rst
if errorlevel 1 ( echo [LOI] Nap STM32 that bai & goto :fail )
echo.
echo --- Nap ESP32 tren %COM% ---
"%ES%" --chip esp32s3 --port %COM% --baud 921600 write-flash ^
  0x0 "%REL%ESP32_RT45_CLEAN_NET45.bootloader.bin" ^
  0x8000 "%REL%ESP32_RT45_CLEAN_NET45.partitions.bin" ^
  0xe000 "%REL%ESP32_RT45_CLEAN_NET45.boot_app0.bin" ^
  0x10000 "%REL%ESP32_RT45_CLEAN_NET45.app.bin"
if errorlevel 1 ( echo [LOI] Nap ESP32 that bai & goto :fail )
echo.
echo ============================================================
echo  XONG. Banner: RT45-CLEAN-NET45 ; nut: 50 / 80 / MAX 90
echo  Serial ESP32 phai co: [NET41] ... -^> AP chN   va   [UART] ... @ 921600 bps
echo  BINFO trong log phai la BAL-20261121-R12-8DIR-RT45-CLEAN
echo  Luot chay dau la nghiem thu: xem muc 4 cua README.
echo ============================================================
pause
exit /b 0
:fail
echo.
echo  Khong nap duoc. Kiem tra day cam, nguon, driver ST-LINK/CP210x, so COM.
pause
exit /b 1
