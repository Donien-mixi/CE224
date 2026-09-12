# CE224 — Đồ Án Xe Cân Bằng 2 Bánh
### (Two-Wheeled Self-Balancing Robot: Tự Đứng + Đua Tốc Độ Cao)

Dự án nghiên cứu, thiết kế và chế tạo Robot 2 bánh tự cân bằng ứng dụng giải thuật **Cascade PID 2 vòng lồng nhau** (Angle PD + Velocity PI) trên vi điều khiển **STM32F411CEU6 (ARM Cortex-M4F)** kết hợp cảm biến quán tính **ICM-20602** và mạch công suất **Dual A4950**.

---

## 📌 Tài liệu cốt lõi

* 📖 **[Lộ trình triển khai 6 giai đoạn (Roadmap)](Roadmap_Xe_Can_Bang_2_Banh_Chuan_Hoa.md)**:
  * Nguyên lý điều khiển con lắc ngược và Cascade PID.
  * Bảng rà soát kỹ thuật linh kiện (BOM).
  * Quy hoạch Timer (TIM1 PWM 20kHz, TIM2/TIM3 Encoder x4, TIM4 ngắt 200Hz).
  * Bộ lọc bù (Complementary Filter $\alpha = 0.96 - 0.98$).
  * Kỹ thuật tune PID thực chiến, chống Integral Windup và các cơ chế an toàn.
* 📁 **[Đặc tả cấu trúc thư mục & mã nguồn dự án](Cau_Truc_Thu_Muc_Do_An.md)**:
  * Quy chuẩn 5 phân hệ: `01_Firmware/`, `02_Hardware/`, `03_Mechanical/`, `04_Tools_Telemetry/`, `05_Docs_Reports/`.
  * Kiến trúc phân tầng phần mềm nhúng (HAL $\rightarrow$ BSP $\rightarrow$ Control $\rightarrow$ App).
  * Giao thức truyền thông Telemetry & điều khiển qua Bluetooth.

---

## 🛠 Phần cứng & Cấu hình chính

* **MCU**: STM32F411CEU6 (100MHz, FPU)
* **IMU**: ICM-20602 (SPI / I2C) + Đệm xốp chống rung FPV
* **Động cơ**: 2x GA25-370 (12V, ~300 RPM, hộp số 1:30) + Hall Encoder đếm 2 kênh TI1/TI2
* **Mạch công suất**: Dual A4950 (PWM 20kHz, Star-grounding, điện trở chia áp set dòng VREF)
* **Nguồn**: Pin LiPo 3S (11.1V - 12.6V) + Hạ áp Buck XL4015 (5A) cấp nguồn số
* **Giao tiếp**: Bluetooth HC-05 / JDY-31 (115200 baud)
