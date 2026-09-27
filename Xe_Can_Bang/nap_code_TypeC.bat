@echo off
chcp 65001 >nul
echo ========================================================
echo   TIẾN TRÌNH NẠP CODE STM32F411 QUA CỔNG USB TYPE-C
echo ========================================================
echo.
echo 1. Đảm bảo cáp Type-C đã cắm vào máy tính.
echo 2. Đã bấm 2 nút: Giữ BOOT0 -> Nhấn nhả NRST -> Thả BOOT0.
echo.
pause

echo Đang nạp firmware Xe_Can_Bang.hex vào chip STM32...
"C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe" -c port=usb1 -w "d:\DA_HTN\Xe_Can_Bang\Debug\Xe_Can_Bang.hex" -v

if %errorlevel% equ 0 (
    echo.
    echo ========================================================
    echo   NẠP CODE THÀNH CÔNG 100%!
    echo   Bây giờ bạn hãy:
    echo   1. Rút cáp Type-C ra.
    echo   2. Nhấc bổng bánh xe / kê xe lên.
    echo   3. Cắm pin LiPo và bấm nút NRST để xe chạy!
    echo ========================================================
) else (
    echo.
    echo [LỖI] Không nạp được! Hãy kiểm tra lại:
    echo 1. Bạn đã bấm giữ BOOT0 và bấm nhả NRST để vào DFU chưa?
    echo 2. Driver WinUSB đã cài trên Zadig chưa?
)
echo.
pause
