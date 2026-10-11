# CẨM NANG ĐI DÂY TOÀN BỘ HỆ THỐNG — XE CÂN BẰNG 2 BÁNH TỐC ĐỘ CAO (TWIP ROBOT)
## (BẢN CHUẨN HÓA HOÀN THIỆN 100% — TRIỆT TIÊU NHẬP NHẰNG, CHỐNG CHÁY CHÍP & CHỐNG NHIỄU)

> **QUY ƯỚC HƯỚNG VÀ VỊ TRÍ ROBOT**:
> * Đứng từ **phía sau xe nhìn về phía trước**:
>   * **Bên Trái**: Động cơ Trái + Encoder Trái (`TIM2`) + Kênh Driver A (`AIN1, AIN2, AOUT1, AOUT2`).
>   * **Bên Phải**: Động cơ Phải + Encoder Phải (`TIM3`) + Kênh Driver B (`BIN1, BIN2, BOUT1, BOUT2`).
> * **Cảm biến GY-BMI160**: Đặt ở giữa sàn xe, mặt chip ngửa lên trời, có lót lớp xốp cách rung 2-3mm.

---

## PHẦN 1: NGUYÊN TẮC BẮT BUỘC — TẠI SAO PHẢI TÁCH WAGO VÀ BREADBOARD?

Hệ thống có 2 phân vùng điện áp và dòng điện hoàn toàn khác biệt:

```
                    ┌────────────────────────────────────────────────────────┐
                    │ PHÂN TÁCH ĐIỆN VỰC ĐỘNG LỰC & ĐIỆN VỰC ĐIỀU KHIỂN SỐ   │
                    └────────────────────────────────────────────────────────┘

    ╔════════════════════════════════════════╗        ╔════════════════════════════════════════╗
    ║      KHỐI CÔNG SUẤT CAO (WAGO)         ║        ║       KHỐI TÍN HIỆU SỐ (BREADBOARD)    ║
    ╠════════════════════════════════════════╣        ╠════════════════════════════════════════╣
    ║ • Dòng điện cực lớn: 5A - 10A          ║        ║ • Dòng điện rất nhỏ: 0.1A - 0.5A       ║
    ║ • Điện áp: 12V Pin LiPo & 5V sau Buck  ║        ║ • Điện áp: 5.0V nuôi bo & 3.3V logic   ║
    ║ • Gồm: Giắc XT60 Pin, Driver A4950,    ║        ║ • Gồm: STM32, ESP32, BMI160, Buzzer   ║
    ║   Mạch hạ áp Buck XL4015, Tụ hóa bù áp ║        ║ • Cắm chung trên các lỗ cắm Breadboard ║
    ║ • Cút nối WAGO chịu dòng 20A - 32A     ║        ║ • Lá đồng Breadboard chỉ chịu <= 0.8A  ║
    ╚════════════════════════════════════════╝        ╚════════════════════════════════════════╝
```

> [!CAUTION]
> **CẢNH BÁO NGUY HIỂM CHÁY NỔ**:
> Tuyệt đối **KHÔNG CẮM DÂY 12V TỪ PIN HOẶC DÂY NGUỒN ĐỘNG CƠ VÀO BREADBOARD**! Lá đồng mỏng bên trong Breadboard sẽ nóng đỏ, chảy nhựa cách điện, gây chập cực Pin LiPo (cháy nổ) hoặc sụt áp tức thời làm vi điều khiển STM32/ESP32 bị treo!

---

## PHẦN 2: HƯỚNG DẪN ĐẤU NỐI KHỐI WAGO (BẮT BUỘC THỰC THI)

Hệ thống cần **2 khối WAGO** (mỗi khối gồm 1 cút Dương `+` và 1 cút Âm `-`):

### 1. CỤM WAGO SỐ 1: ĐỘNG LỰC 12V VÀ MASS XẢ PIN (DÂY TO 18AWG)
*Chuyên trách dẫn dòng điện cực đại từ Pin LiPo 3S (11.1V – 12.6V).*

#### A. Cút WAGO 1 Dương — Màu Đỏ (12V Pin):
* **Cổng Vào**: Dây ĐỎ to (18AWG) từ đầu cái giắc **XT60 của Pin LiPo 3S**.
* **Cổng Ra 1**: Dây ĐỎ to nối vào chân **`VM` (Hàng 1)** của Driver Dual A4950.
* **Cổng Ra 2**: Dây ĐỎ nối vào cọc **`IN (+)`** của Mạch Buck XL4015.

#### B. Cút WAGO 1 Âm — Màu Đen (Mass xả Pin 12V):
* **Cổng Vào**: Dây ĐEN to (18AWG) từ đầu cái giắc **XT60 của Pin LiPo 3S**.
* **Cổng Ra 1**: Dây ĐEN to nối vào chân **`GND` (Hàng 1)** của Driver Dual A4950.
* **Cổng Ra 2**: Dây ĐEN nối vào cọc **`IN (-)`** của Mạch Buck XL4015.

---

### 2. CỤM WAGO SỐ 2: PHÂN PHỐI NGUỒN 5.0V SẠCH SAU HẠ ÁP XL4015
*Nhận nguồn điện áp ổn định 5.00V sạch từ Mạch Buck XL4015 để chia cho hệ thống điều khiển.*

#### A. Cút WAGO 2 Dương — Màu Đỏ (5.0V Sạch):
* **Cổng Vào**: Dây ĐỎ từ cọc **`OUT (+)`** của Mạch hạ áp Buck XL4015 *(vặn chiết áp đo đúng 5.00V)*.
* **Cổng Ra 1**: Dây ĐỎ cắm thẳng sang **[ĐƯỜNG RAY 5V (+)]** trên Breadboard.
* **Cổng Ra 2**: Dây ĐỎ nối sang chân **`VCC` (Hàng 1)** của Driver Dual A4950 *(cấp điện áp tham chiếu $V_{REF}=5.0\text{V}$ để mở dòng tối đa 2.0A cho động cơ)*.
* **Cổng Ra 3**: Chân Dương (+) của **Tụ hóa $1000\mu F / 16V$** *(chống sụt áp khi ESP32 phát Wi-Fi)*.

#### B. Cút WAGO 2 Âm — Màu Đen (Mass 5V / GND Hệ Thống):
* **Cổng Vào**: Dây ĐEN từ cọc **`OUT (-)`** của Mạch hạ áp Buck XL4015.
* **Cổng Ra 1**: Dây ĐEN cắm thẳng sang **[ĐƯỜNG RAY GND (-)]** trên Breadboard *(cung cấp mốc 0V chuẩn cho toàn bộ vi mạch số)*.
* **Cổng Ra 2**: Chân Âm (-) của **Tụ hóa $1000\mu F / 16V$** *(chân có vạch sọc trắng/xám)*.

---

## PHẦN 3: ĐI DÂY 4 LINH KIỆN TRÊN BREADBOARD (CHUNG NGUỒN & CHUNG MASS)

Cả 4 linh kiện cắm trên Breadboard gồm:
1. **STM32F411CEU6 Black Pill** (Vi điều khiển trung tâm)
2. **Gateway ESP32-S3 N16R8** (Web HUD, Wi-Fi Telemetry)
3. **Module GY-BMI160** (Cảm biến con quay hồi chuyển & gia tốc góc)
4. **Module Còi Active Buzzer** (Báo động trạng thái)

```
===================================================================================================
                       SƠ ĐỒ PHÂN BỔ ĐƯỜNG RAY NGUỒN & MASS TRÊN BREADBOARD
===================================================================================================

       Dây ĐỎ 5V từ WAGO 2 ───────────────┐
       Dây ĐEN GND từ WAGO 2 ─────────┐   │
                                      │   │
                                      ▼   ▼
  🔴 ĐƯỜNG RAY 5V (+) [Vạch Đỏ]: ─────┴───┼───────────────────────────────────────────────
                                          │
  🔵 ĐƯỜNG RAY GND (-) [Vạch Xanh/Đen]: ──┴───────────────────────────────────────────────


  [A] NHỮNG CHÂN CẮM CHUNG VÀO ĐƯỜNG RAY 5V (+):
      ├── 1. Chân [5V] của bo mạch STM32F411 Black Pill.
      ├── 2. Chân [5V] (hoặc VIN) của bo mạch Gateway ESP32-S3.
      └── 3. Chân [VCC] (hoặc +) của Module Còi Buzzer.
      ⛔ (CẤM: KHÔNG CẮM CHÂN 3V3 HOẶC VIN CỦA CẢM BIẾN BMI160 VÀO RAY 5V NÀY!)

  [B] NHỮNG CHÂN CẮM CHUNG VÀO ĐƯỜNG RAY GND (-):
      ├── 1. Chân [GND] của STM32F411 Black Pill.
      ├── 2. Chân [GND] của Gateway ESP32-S3.
      ├── 3. Chân [GND] (hoặc -) của Module Còi Buzzer.
      ├── 4. Chân [GND] của Module GY-BMI160.
      ├── 5. Chân [SA0] của Module GY-BMI160 (Kéo xuống GND để khóa địa chỉ I2C = 0x68).
      ⛔ (CẤM: KHÔNG CẮM DÂY GND HÀNG 2 CỦA DRIVER A4950 VÀO RAY GND NÀY — xem Phần 6 mục 1)
      ├── 6. Dây ĐEN (Pin 2 - Hall GND) của Encoder Động cơ Trái.
      └── 7. Dây ĐEN (Pin 2 - Hall GND) của Encoder Động cơ Phải.

  [C] TRẠM NGUỒN 3.30V SẠCH CỦA STM32 (DÀNH RIÊNG CHO THIẾT BỊ 3.3V TRÊN BREADBOARD):
      Chân [3.3V] trên bo mạch STM32 Black Pill ──────┐
                                                      │ (Cắm vào 1 hàng rãnh rỗng trên bo test)
                                                      ▼
      ╔═══════════════════════════════════════════════════════════════════════════════╗
      ║              TRẠM NGUỒN 3.30V AN TOÀN (LẤY TỪ IC ỔN ÁP LDO STM32)             ║
      ╠═══════════════════════════════════════════════════════════════════════════════╣
      ├── 1. Chân [3V3] của GY-BMI160 (Nuôi trực tiếp chip BMI160 đúng áp chuẩn 3.30V)║
      ├── 2. Chân [CS] của GY-BMI160 (Kéo lên mức HIGH để chip chuyển sang mode I2C)  ║
      ├── 3. Dây XANH DƯƠNG (Pin 5 - Hall VCC) của Encoder Bánh Trái                  ║
      └── 4. Dây XANH DƯƠNG (Pin 5 - Hall VCC) của Encoder Bánh Phải                  ║
      ╚═══════════════════════════════════════════════════════════════════════════════╝
===================================================================================================
```

> [!CAUTION]
> **VẤN ĐỀ CỐT TỬ CỦA CẢM BIẾN GY-BMI160**:
> 1. Chip Bosch BMI160 là vi mạch siêu nhạy cảm với điện áp: **Điện áp định mức là 3.3V, tuyệt đối không được vượt quá 3.6V**!
> 2. Chân **`VIN`** trên vỉ GY-BMI160 đi qua một con diode và IC hạ áp dỏm gây sụt áp, làm đường I2C tụt xuống 2.36V khiến STM32 không đọc được $\to$ **BỎ TRỐNG CHÂN `VIN`**.
> 3. Chân **`3V3`** ăn thẳng vào ruột chip. **BẮT BUỘC PHẢI LẤY TỪ CHÂN `3.3V` CỦA STM32** (tuyệt đối không cắm vào Ray 5V!).
> 4. Chân **`SA0`** (một số vỉ ghi `SDO`): **BẮT BUỘC CẮM RAY GND** để chốt địa chỉ I2C = `0x68`. Nếu thả nổi, địa chỉ sẽ đổi thành `0x69` $\to$ Còi hú liên tục báo lỗi I2C!
> 5. Chân **`CS`**: **BẮT BUỘC CẮM VÀO CHÂN `3.3V` CỦA STM32** để chip chọn chuẩn I2C. Nếu thả nổi hoặc chạm mass, chip sẽ tự động chuyển sang SPI và ngắt I2C $\to$ Liệt cảm biến!

---

## PHẦN 4: KẾT NỐI TÍN HIỆU GIỮA 4 LINH KIỆN TRÊN BREADBOARD

Tất cả các kết nối tín hiệu dưới đây là dây nhảy ngắn cắm trực tiếp trên Breadboard:

```
                            SƠ ĐỒ TÍN HIỆU NỘI BỘ BREADBOARD
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                        │
│  [STM32F411 BLACK PILL]                             [MODULE BOSCH GY-BMI160]           │
│      Chân PB8 (I2C1_SCL) ─────────────────────────────► Chân SCL                       │
│      Chân PB9 (I2C1_SDA) ─────────────────────────────► Chân SDA                       │
│      Chân PB2 (EXTI2)    ◄───────────────────────────── Chân INT1                      │
│                                                                                        │
│  [STM32F411 BLACK PILL]                             [GATEWAY ESP32-S3]                 │
│      Chân PB6 (USART1_TX) ────────────────────────────► Chân GPIO18 (RX)               │
│      Chân PB7 (USART1_RX) ◄──────────────────────────── Chân GPIO17 (TX)               │
│                                                                                        │
│  [STM32F411 BLACK PILL]                             [MODULE CÒI ACTIVE BUZZER]         │
│      Chân PB12 (GPIO Output) ─────────────────────────► Chân IN (hoặc S / I/O)         │
│                                                                                        │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## PHẦN 5: BẢNG TRA CỨU CHI TIẾT TỪNG CHÂN (PINOUT REFERENCE)

### 1. Vi điều khiển STM32F411CEU6 Black Pill (Cắm trên Breadboard)

| Tên Chân STM32 | Loại Tín Hiệu | Cắm Sang Đâu? | Vị Trí Cụ Thể | Ý Nghĩa Kỹ Thuật |
| :---: | :---: | :--- | :---: | :--- |
| **`5V`** | Nguồn Vào | Đường ray Breadboard | **Ray 5V (+)** | Nhận 5.00V sạch từ Buck XL4015 (qua Wago 2) |
| **`GND`** | Nguồn Mass | Đường ray Breadboard | **Ray GND (-)** | Mốc 0V chuẩn toàn hệ thống |
| **`3.3V`**| Nguồn Ra | Breadboard | **Trạm 3.3V** | Cấp áp 3.30V chuẩn nuôi BMI160 và 2 Encoder |
| **`PB8`** | I2C1 Clock | Module BMI160 | Chân **`SCL`** | Xung nhịp Fast Mode 400kHz |
| **`PB9`** | I2C1 Data | Module BMI160 | Chân **`SDA`** | Tuyến dữ liệu I2C |
| **`PB2`** | Ngắt ngoài | Module BMI160 | Chân **`INT1`** | EXTI2: Báo dữ liệu góc mới sẵn sàng (200Hz) |
| **`PB6`** | UART1 TX | Gateway ESP32-S3 | Chân **`GPIO18`** | Bắn dữ liệu Telemetry 100Hz lên Web HUD |
| **`PB7`** | UART1 RX | Gateway ESP32-S3 | Chân **`GPIO17`** | Nhận lệnh điều khiển và tinh chỉnh PID |
| **`PB12`**| Digital Out | Module Còi Buzzer | Chân **`IN`** | Bật/tắt còi báo trạng thái FSM |
| **`PA8`** | TIM1_CH1 | Driver Dual A4950 | Chân **`AIN1`** (Hàng 1)| PWM 20kHz Bánh Trái Tiến |
| **`PA9`** | TIM1_CH2 | Driver Dual A4950 | Chân **`AIN2`** (Hàng 1)| PWM 20kHz Bánh Trái Lùi |
| **`PA10`**| TIM1_CH3 | Driver Dual A4950 | Chân **`BIN1`** (Hàng 2)| PWM 20kHz Bánh Phải Tiến |
| **`PA11`**| TIM1_CH4 | Driver Dual A4950 | Chân **`BIN2`** (Hàng 2)| PWM 20kHz Bánh Phải Lùi *(Xem lưu ý nạp Type-C bên dưới)* |
| **`PA0`** | TIM2_CH1 | Encoder Bánh Trái | Dây **Vàng** (Pin 3) | Đọc xung Hall Kênh A Bánh Trái |
| **`PA1`** | TIM2_CH2 | Encoder Bánh Trái | Dây **Xanh Lá** (Pin 4)| Đọc xung Hall Kênh B Bánh Trái |
| **`PB4`** | TIM3_CH1 | Encoder Bánh Phải | Dây **Vàng** (Pin 3) | Đọc xung Hall Kênh A Bánh Phải |
| **`PB5`** | TIM3_CH2 | Encoder Bánh Phải | Dây **Xanh Lá** (Pin 4)| Đọc xung Hall Kênh B Bánh Phải |
| **`PC13`**| LED Onboard| *Có sẵn trên bo* | — | Đèn báo: Khởi động (2.5s) $\to$ Standby (chậm) $\to$ Run (sáng) |

> [!TIP]
> **NẠP CODE BẰNG MẠCH NẠP ST-LINK V2 (KHUYẾN NGHỊ - XEM MỤC 5 BÊN DƯỚI)**:
> Dùng **ST-Link V2** cắm vào 4 chân `3V3`, `GND`, `SWDIO`, `SWCLK` của Black Pill. Vì ST-LINK dùng giao thức **SWD** (không dùng cổng Type-C) nên:
> * **KHÔNG cần rút dây `PA11`** ra khỏi Driver A4950.
> * **KHÔNG cần bấm `BOOT0`/`NRST`** để vào chế độ nạp.
> * Bấm `Run/Debug` trong STM32CubeIDE là nạp ngay, nạp lại bao nhiêu lần cũng mượt.

> [!WARNING]
> **LƯU Ý VỀ CHÂN `PA11` (CHỈ KHI NẠP BẰNG CÁP TYPE-C/DFU)**:
> Chân **`PA11`** của STM32 đồng thời là chân tín hiệu **`USB_DM` (D-)** của cổng Type-C. Nếu buộc phải nạp bằng cáp Type-C qua DFU (không có ST-LINK) thì **bắt buộc RÚT DÂY `PA11`** ra khỏi Driver A4950, nếu không máy tính sẽ báo *"USB device not recognized"*.

---

### 2. Cảm biến quán tính Bosch GY-BMI160 (Cắm trên Breadboard — 6 Chân Chuẩn Hóa Đã Test Thực Tế)

| Tên Chân BMI160 | Nối Sang Đâu? | Vị Trí Cụ Thể | Quy Tắc Bắt Buộc (Đã Test Thực Nghiệm) |
| :---: | :--- | :---: | :--- |
| **`3V3`** | STM32 Black Pill | Chân **`3.3V` STM32** | **BẮT BUỘC**: Cấp đúng 3.30V vào ruột chip. **TUYỆT ĐỐI KHÔNG CẮM VÀO 5V** |
| **`GND`** | Đường ray Breadboard | **Ray GND (-)** | Nối chung mass toàn hệ thống |
| **`SCL`** | STM32 Black Pill | Chân **`PB8`** | Tuyến xung nhịp I2C1 Clock (Fast Mode 400kHz) |
| **`SDA`** | STM32 Black Pill | Chân **`PB9`** | Tuyến dữ liệu I2C1 Data |
| **`CS`** | STM32 Black Pill | Chân **`3.3V` STM32** | **BẮT BUỘC**: Kéo lên HIGH (3.3V) để ép chip chọn chế độ I2C (thay vì SPI) |
| **`SA0`** | Đường ray Breadboard | **Ray GND (-)** | **BẮT BUỘC**: Nối Mass để chốt cố định địa chỉ I2C = `0x68` |
| **`VIN`** | ⛔ **BỎ TRỐNG HOÀN TOÀN** | — | **CẤM CẮM DÂY VÀO CHÂN NÀY** *(Đi qua diode/LDO dỏm gây sụt áp I2C làm đơ chip)* |
| **`INT1`**| STM32 Black Pill | Chân **`PB2`** | Ngắt ngoài EXTI2 chu kỳ 200Hz (Tùy chọn) |
| **`INT2`**| *BỎ TRỐNG* | — | Không sử dụng |

---

### 3. Gateway Không Dây ESP32-S3 N16R8 (Cắm trên Breadboard)

| Tên Chân ESP32-S3 | Nối Sang Đâu? | Vị Trí Cụ Thể | Chức Năng |
| :---: | :--- | :---: | :--- |
| **`5V`** *(hoặc `VIN`)*| Đường ray Breadboard | **Ray 5V (+)** | Nhận 5.0V nuôi ESP32 |
| **`GND`** | Đường ray Breadboard | **Ray GND (-)** | Mass chung vi mạch |
| **`GPIO18`** *(RX)* | STM32 Black Pill | Chân **`PB6`** (TX) | Nhận gói Telemetry 100Hz từ STM32 |
| **`GPIO17`** *(TX)* | STM32 Black Pill | Chân **`PB7`** (RX) | Gửi lệnh điều khiển từ Web HUD xuống STM32 |

---

### 4. Module Còi Báo Động Active Buzzer (Cắm trên Breadboard)

* **Loại Module Còi 3 chân (khuyên dùng)**:
  - Chân **`VCC`** (hoặc `+`) $\to$ Cắm vào **Ray 5V (+)** của Breadboard.
  - Chân **`GND`** (hoặc `-`) $\to$ Cắm vào **Ray GND (-)** của Breadboard.
  - Chân **`IN`** (hoặc `S`) $\to$ Cắm vào chân **`PB12`** của STM32 Black Pill.
* **Loại Còi chíp rời 2 chân**:
  - Chân Dài (`+`) $\to$ Cắm vào chân **`PB12`** của STM32 Black Pill.
  - Chân Ngắn (`-`) $\to$ Cắm vào **Ray GND (-)** của Breadboard.

---

### 5. Mạch nạp ST-Link V2 (Cắm ngoài Breadboard — Dùng để nạp và gỡ lỗi)

Kết nối tối thiểu **4 dây** (đấu theo **nhãn in trên mạch nạp**, không cần đếm số chân):

| Chân ST-Link V2 | Nhãn trên mạch ST-Link | Nối tới STM32 Black Pill | Ghi chú |
| :---: | :---: | :--- | :--- |
| **3.3V** | `3.3V` (hoặc `3V3`) | Chân **`3.3V`** | **CHỈ cắm khi board CHƯA có nguồn riêng** |
| **GND** | `GND` | Chân **`GND`** | **Bắt buộc** nối chung mass |
| **SWDIO** | `SWDIO` (hoặc `DIO`) | Chân **`PA13`** (SWDIO) | Tuyến dữ liệu nạp/gỡ lỗi |
| **SWCLK** | `SWCLK` (hoặc `CLK`) | Chân **`PA14`** (SWCLK) | Tuyến xung nhịp nạp/gỡ lỗi |
| *(tùy chọn)* **RST** | `RST` | Chân **`NRST`** / `R` | Tăng độ ổn định khi chọn *"Connect under reset"* |

```
   [ST-Link V2]                         [STM32F411 BLACK PILL]
    3.3V  ───────────────────────────────  3.3V   (chỉ khi board chưa có nguồn)
    GND   ───────────────────────────────  GND
    SWDIO ───────────────────────────────  PA13 (SWDIO)
    SWCLK ───────────────────────────────  PA14 (SWCLK)
    RST   ───────────────────────────────  NRST   (tùy chọn, nên nối)
```

> [!CAUTION]
> **TRÁNH CẤP NGUỒN TRÙNG**:
> Nếu xe đang cắm Pin LiPo (board đã có nguồn riêng), **TUYỆT ĐỐI KHÔNG nối dây `3.3V` từ ST-LINK** vào board để tránh hai nguồn đấu nhau gây hỏng. Khi đó chỉ nối **`GND`, `SWDIO`, `SWCLK`** (và `RST` nếu có). Chỉ nối `3.3V` khi muốn ST-LINK nuôi board lúc board chưa có nguồn.

> [!NOTE]
> **QUY TRÌNH NẠP**: Cắm ST-LINK vào cổng USB máy tính → mở STM32CubeIDE → chọn đúng cấu hình `Xe_Can_Bang Debug` (ST-LINK, SWD) → bấm **Run/Debug** (`F11`). CubeIDE tự build rồi nạp, **không cần bấm `BOOT0`** và **không cần rút `PA11`**.

---

## PHẦN 6: DÂY NỐI LINH KIỆN NGOÀI BREADBOARD (DRIVER VÀ 2 ĐỘNG CƠ)

### 1. Mạch Công Suất Driver Dual A4950 (Gắn trên khung xe)

* **Hàng chân 1 (Cạnh có tụ điện)**:
  - **`VM`** $\to$ Nối vào **Cọc Dương (+) WAGO 1 (12V Pin LiPo)** *(Dây Đỏ to)*.
  - **`GND`** $\to$ Nối vào **Cọc Âm (-) WAGO 1 (Mass xả Pin)** *(Dây Đen to)*.
  - **`VCC`** $\to$ Nối vào **Cọc Dương (+) WAGO 2 (5.0V)** *(hoặc Ray 5V Breadboard)* để cấp điện áp tham chiếu $V_{REF}=5.0\text{V}$.
  - **`AIN2`** $\to$ Nối vào chân **`PA9`** của STM32 (Bánh Trái Lùi).
  - **`AIN1`** $\to$ Nối vào chân **`PA8`** của STM32 (Bánh Trái Tiến).
  - **`BOUT1`** $\to$ Nối vào Dây **ĐỎ** (Pin 1) của Động cơ Bánh Phải *(Xoắn đôi với BOUT2)*.
  - **`BOUT2`** $\to$ Nối vào Dây **TRẮNG** (Pin 6) của Động cơ Bánh Phải *(Xoắn đôi với BOUT1)*.

* **Hàng chân 2 (Hàng đối diện)**:
  - **`AOUT2`** $\to$ Nối vào Dây **TRẮNG** (Pin 6) của Động cơ Bánh Trái *(Xoắn đôi với AOUT1)*.
  - **`AOUT1`** $\to$ Nối vào Dây **ĐỎ** (Pin 1) của Động cơ Bánh Trái *(Xoắn đôi với AOUT2)*.
  - **`BIN1`** $\to$ Nối vào chân **`PA10`** của STM32 (Bánh Phải Tiến).
  - **`BIN2`** $\to$ Nối vào chân **`PA11`** của STM32 (Bánh Phải Lùi).
  - **`GND`** $\to$ Nối 1 dây riêng biệt về **Cọc Âm (-) WAGO 2 (Mass 5V)** *(KHÔNG CẮM VÀO RAY GND BREADBOARD! Dòng hồi tiếp PWM 20kHz có spike lên 0.5–2A, nếu chạy chung Ray GND Breadboard sẽ gây nhiễu Ground Bounce ~50mV làm méo tín hiệu I2C của BMI160)*.

> [!CAUTION]
> **CHỐNG SỤT ÁP & NHIỄU ĐẤT (GROUND BOUNCE)**:
> Dây GND Hàng 2 của Driver A4950 mang dòng hồi tiếp PWM 20kHz (spike đỉnh 0.5–2A). Nếu cắm vào Ray GND Breadboard (trở kháng lá đồng ~100mΩ), xung dòng sẽ tạo nhiễu điện áp ~50mV trên mốc 0V chung, gây rung tín hiệu I2C của BMI160 → Gyro đọc sai ±1–3 deg/s. **BẮT BUỘC** nối dây này về **WAGO 2 (−)** để cách ly hoàn toàn đường mass công suất và đường mass tín hiệu số.

---

### 2. Hai Động cơ GA25-370 có Encoder (Bánh Trái & Bánh Phải)

Đuôi mỗi động cơ GA25 có giắc JST PH2.0 gồm 6 chân (đếm từ Pin 1 đến Pin 6):

| Chân & Màu Dây Chuẩn | Ký Hiệu | Bản Chất Dây | Điểm Đến | Vị Trí Cắm |
| :---: | :---: | :---: | :--- | :---: |
| **Pin 1 — 🔴 ĐỎ** | M1 | Nguồn Motor 12V | Driver Dual A4950 | **`AOUT1`** *(Trái)* / **`BOUT1`** *(Phải)* |
| **Pin 6 — ⚪ TRẮNG**| M2 | Nguồn Motor 12V | Driver Dual A4950 | **`AOUT2`** *(Trái)* / **`BOUT2`** *(Phải)* |
| **Pin 2 — ⚫ ĐEN** | GND | Mass Cảm biến Hall | Breadboard | **Ray GND (-)** |
| **Pin 5 — 🔵 XANH DƯƠNG**| VCC | Nguồn Hall 3.3V | STM32 Black Pill | Chân **`3.3V` STM32** |
| **Pin 3 — 🟡 VÀNG** | C1 | Xung Hall Kênh A | STM32 Black Pill | Chân **`PA0`** *(Trái)* / **`PB4`** *(Phải)* |
| **Pin 4 — 🟢 XANH LÁ**| C2 | Xung Hall Kênh B | STM32 Black Pill | Chân **`PA1`** *(Trái)* / **`PB5`** *(Phải)* |

> [!CAUTION]
> **CẢNH BÁO ĐẶC BIỆT VỀ MÀU DÂY ENCODER GA25**:
> * **Dây ĐỎ (Pin 1) và Dây TRẮNG (Pin 6)** là 2 dây cấp điện 12V cho cuộn dây động cơ. Hai dây này **bắt buộc xoắn chặt vào nhau (2 - 3 vòng/cm)** trước khi đấu vào Driver để triệt tiêu bức xạ điện từ EMI gây nhiễu I2C!
> * Dây **XANH LÁ (Pin 4)** là dây tín hiệu xung Kênh B. Tuyệt đối **không nhầm lẫn giữa dây Trắng (12V) và dây Xanh Lá (3.3V)**!

---

## PHẦN 7: BẢNG KIỂM TRA ĐIỆN ÁP TRƯỚC KHI BẬT NGUỒN PIN (CHECKLIST)

Sau khi đi dây xong, bạn hãy rút giắc động cơ, cầm đồng hồ vạn năng VOM (thang đo DC Volts) kiểm tra tuần tự 4 bước:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        QUY TRÌNH ĐO KIỂM BẰNG ĐỒNG HỒ VOM                              │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 1. Đo cọc OUT(+) và OUT(-) của Buck XL4015:  Phải đúng: 5.00V - 5.05V                 │
│ 2. Đo Ray 5V và Ray GND trên Breadboard:      Phải đúng: 5.00V                         │
│ 3. Đo chân 3V3 của Module BMI160 và Ray GND:  Phải đúng: 3.28V - 3.30V (Tuyệt đối < 3.6V)│
│ 4. Đo chân CS của BMI160:                     Phải đúng: 3.30V                         │
│ 5. Đo chân SA0 của BMI160:                    Phải đúng: 0.00V (Thông mạch với GND)    │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## PHẦN 8: XỬ LÝ CHIỀU QUAY ĐỘNG CƠ KHI CHẠY THẬT

Sau khi cắm nguồn Pin và dựng thẳng xe:
1. **Kiểm tra phản hồi giữ thăng bằng**:
   * Nghiêng thân xe về phía **TRƯỚC** $\to$ Hai bánh xe phải cùng quay **TIẾN** để đỡ xe.
   * Nghiêng thân xe về phía **SAU** $\to$ Hai bánh xe phải cùng quay **LÙI** để đỡ xe.
2. **Nếu có 1 bánh bị quay ngược chiều (xe bị xoay tròn tại chỗ)**:
   * Nếu **Bánh Trái** quay ngược: Đảo chéo 2 dây **`AOUT1`** và **`AOUT2`** trên Driver A4950.
   * Nếu **Bánh Phải** quay ngược: Đảo chéo 2 dây **`BOUT1`** và **`BOUT2`** trên Driver A4950.
   *(Không cần sửa hay nạp lại bất kỳ dòng code nào!)*
