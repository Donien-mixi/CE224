@echo off
setlocal EnableDelayedExpansion
title NAP RT44-TURN / NET44
set "REL=%~dp0"
set "CP=C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe"
set "ES=C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
echo ============================================================
echo  NAP RT44-TURN / NET44  (cua deu 4 huong (tran yaw theo duty))
echo  Nap CA HAI board (lease lenh doi tren STM32, header log doi tren ESP32).
echo ============================================================
echo.
if not exist "%CP%" ( echo [LOI] Khong thay STM32_Programmer_CLI.exe & goto :fail )
if not exist "%ES%" ( echo [LOI] Khong thay esptool.exe & goto :fail )
if not exist "%REL%STM32_BAL_R12_RT44_TURN.hex" ( echo [LOI] Thieu file hex & goto :fail )
if not exist "%REL%ESP32_RT44_TURN_NET44.app.bin" ( echo [LOI] Thieu file app.bin & goto :fail )
set /p COM=So COM cua ESP32 (vi du COM3, BAT BUOC):
if "%COM%"=="" ( echo [LOI] Ban nay phai nap ca ESP32. & goto :fail )
echo.
echo --- Nap STM32 ---
"%CP%" -c port=SWD -w "%REL%STM32_BAL_R12_RT44_TURN.hex" -v -rst
if errorlevel 1 ( echo [LOI] Nap STM32 that bai & goto :fail )
echo.
echo --- Nap ESP32 tren %COM% ---
"%ES%" --chip esp32s3 --port %COM% --baud 921600 write-flash ^
  0x0 "%REL%ESP32_RT44_TURN_NET44.bootloader.bin" ^
  0x8000 "%REL%ESP32_RT44_TURN_NET44.partitions.bin" ^
  0xe000 "%REL%ESP32_RT44_TURN_NET44.boot_app0.bin" ^
  0x10000 "%REL%ESP32_RT44_TURN_NET44.app.bin"
if errorlevel 1 ( echo [LOI] Nap ESP32 that bai & goto :fail )
echo.
echo ============================================================
echo  XONG. Banner: RT44-TURN-NET44 ; nut: 50 / 80 / MAX 90
echo  Serial ESP32 phai co: [NET41] ... -^> AP chN   va   [UART] ... @ 921600 bps
echo  Log moi se co dong: # RADIO ch=.. rssi=.. phy=.. (xem muc 5 cua README)
echo ============================================================
pause
exit /b 0
:fail
echo.
echo  Khong nap duoc. Kiem tra day cam, nguon, driver ST-LINK/CP210x, so COM.
pause
exit /b 1
