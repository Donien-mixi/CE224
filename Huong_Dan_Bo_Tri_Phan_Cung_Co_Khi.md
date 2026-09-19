# HƯỚNG DẪN BỐ TRÍ PHẦN CỨNG & CƠ KHÍ XE TỰ CÂN BẰNG DIY (2 TẦNG)

Bản vẽ kỹ thuật và sơ đồ bố trí không gian 3D giúp bạn sắp xếp các linh kiện đã mua lên 2 tấm mica DIY một cách khoa học, tối ưu trọng tâm (Center of Gravity - CoG) và triệt tiêu nhiễu cơ học/điện từ.

---

## 1. Bản vẽ Sơ Đồ Bố Trí Phần Cứng Khung DIY 2 Tầng

```
                            [TẦNG 2: NÃO BỘ & CẢM BIẾN]
       ┌─────────────────────────────────────────────────────────────┐
       │  [Công tắc nguồn]   [STM32F411]     [GY-BMI160]   [ESP32-S3]│
       │    (Bập bênh)       (Black Pill)   (Đệm xốp 3M)   (N16R8)   │
       │                                     [Buzzer PB12]           │
       └──────────────────────────────┬──────────────────────────────┘
                                      │ 4 cọc đồng M3 lục giác (6cm)
                                      │
                            [TẦNG 1: ĐỘNG LỰC & NGUỒN]
       ┌──────────────────────────────┴──────────────────────────────┐
       │   [Pin LiPo 3S 11.1V]   [Driver A4950]    [Buck XL4015 5A]  │
       └───┬─────────────────────────────────────────────────────┬───┘
           │                                                     │
   [Gá kim loại L] ── [Motor Trái GA25]         [Motor Phải GA25] ── [Gá kim loại L]
   [Khớp đồng lục giác]                         [Khớp đồng lục giác]
   [Bánh cao su 65mm]                           [Bánh cao su 65mm]
```

---

## 2. Nguyên Lý Bố Trí Từng Tầng (Tier Breakdown)

### TẦNG 1: TẦNG ĐỘNG LỰC, CÔNG SUẤT & NGUỒN ĐIỆN (Tầng nặng - Đáy)
> **Mục tiêu**: Tập trung các khối sinh nhiệt, khối nặng và dòng điện lớn ở tầng dưới cùng nhằm hạ thấp vừa đủ trọng tâm và làm ngắn đường dây động cơ.

* **Mặt dưới tấm Mica tầng 1**:
  * **2 Động cơ GA25-370 (12V 280RPM)**: Bắt vít cố định vào 2 gá kim loại chữ L (25mm). Đặt 2 trục động cơ **thẳng hàng 100%** trên một trục ngang đi qua tâm của tấm mica.
  * **Khớp nối đồng lục giác 4mm & 2 Bánh xe 65mm**: Siết chặt ốc chí (set screw) vào mặt vát D của trục motor để chống trượt bánh khi đảo chiều gắt.
* **Mặt trên tấm Mica tầng 1**:
  * **Khối Pin LiPo 3S 11.1V**: Đặt nằm ngang hoặc đứng sát tâm trục bánh xe. Pin là vật thể nặng nhất ($\approx 150 - 180\text{g}$), việc đặt ngay trên trục bánh xe giúp xe có quán tính xoay vừa phải, motor không bị quá tải khi phanh gấp.
  * **Mạch công suất Driver Dual A4950**: Đặt ở khoảng giữa 2 động cơ để đường dây nối ra motor ngắn nhất ($< 6\text{cm}$), giảm thiểu bức xạ nhiễu điện từ (EMI). Nhận 4 đường PWM từ Timer 1 (`PA8`..`PA11`).
  * **Mạch hạ áp Buck XL4015 (5A)**: Bắt vít cách ly ở mép sau tầng 1. Núm xoay tinh chỉnh hạ áp từ 12V xuống đúng **5.0V** trước khi cấp lên tầng 2.

---

### TẦNG 2: TẦNG NÃO BỘ, CẢM BIẾN & GIAO TIẾP (Tầng xử lý - Đỉnh)
> **Mục tiêu**: Cách ly hoàn toàn rung chấn cơ học và bức xạ điện từ từ motor, tạo môi trường "yên tĩnh" nhất cho vi điều khiển và cảm biến quán tính.
*Kết nối với tầng 1 bằng 4 cọc đồng M3 lục giác dài 6cm ở 4 góc.*

* **Cảm biến Bosch BMI160 (Module GY-BMI160)**:
  * **Vị trí bắt buộc**: Đặt tại **CHÍNH GIỮA TRỌNG TÂM HÌNH HỌC**, nằm ngay trên trục thẳng đứng đi qua tâm của 2 bánh xe.
  * **Chống rung**: Dán module lên **miếng xốp đệm FPV / băng keo xốp 3M dày 2-3mm**. Tuyệt đối không bắt vít cứng trực tiếp lên mica vì rung động kim loại từ hộp số GA25 sẽ làm cảm biến gia tốc bị bão hòa nhiễu.
  * **Hướng trục**: Trục $Y$ hoặc $X$ hướng thẳng về phía trước theo chiều tiến của xe.
* **Vi điều khiển STM32F411CEU6 (Black Pill)**:
  * Đặt cạnh BMI160 để các đường dây I2C (`PB8`, `PB9`) thật ngắn ($< 5\text{cm}$), chống suy hao tín hiệu Fast Mode 400kHz.
  * Hướng cổng USB Type-C ra phía sau hoặc cạnh bên để cắm cáp nạp/debug thuận tiện mà không phải tháo khung.
* **Module Gateway Không Dây ESP32-S3 N16R8 (Wi-Fi / BLE)**:
  * Đặt ở mép ngoài cùng của tầng 2. Hướng cụm ăng-ten PCB/ăng-ten gốm chìa ra ngoài, tránh bị kim loại hoặc cọc đồng che khuất sóng RF Wi-Fi/BLE.
  * Cấp nguồn 5V sạch từ ngõ ra mạch Buck XL4015 vào chân 5V/VIN của board ESP32-S3, nối chung mass GND; kết nối 2 chân UART (TX/RX) bắt chéo sang STM32F411 (`PB7`/`PB6`).
* **Công tắc nguồn chính & Còi Buzzer (PB12)**:
  * Đặt ở vị trí dễ thao tác khẩn cấp bằng tay khi xe mất thăng bằng.

---

## 3. Quy Tắc Đi Dây (Wiring & Cable Routing) Để Tránh Nhiễu

1. **Tách biệt đường dây Động lực và Tín hiệu**:
   * Tuyệt đối không bó chung bó dây động cơ (dòng lớn, xung PWM 20kHz) chung với bó dây I2C của BMI160.
   * Dây Encoder (kênh A, B) nên xoắn đôi (Twisted-pair) để chống nhiễu bắt sóng từ cuộn cảm motor.
2. **Điểm nối Mass (Single Point Star GND)**:
   * Chân GND của Pin, GND của Buck XL4015, GND của Driver Dual A4950, GND của STM32 và GND của BMI160 phải quy tụ về một điểm chung để tránh hiện tượng vòng lặp đất (Ground Loop).
3. **Độ chùng của dây nối giữa 2 tầng**:
   * Dây nối từ tầng 1 lên tầng 2 (nguồn 5V, tín hiệu PWM, Encoder) cần có độ chùng vừa phải (không căng cứng) để không truyền rung động cơ học từ tầng 1 lên tầng 2.
