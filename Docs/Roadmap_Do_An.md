# ROADMAP THỰC THI ĐỒ ÁN — XE CÂN BẰNG HAI BÁNH (TWIP ROBOT)

> **Phạm vi tài liệu:** Roadmap **phần mềm, phương pháp & quản trị** để đưa xe hoạt động tốt qua **2 giai đoạn lớn**:
> **(GĐ1)** Tự động cân bằng — chống nhiễu; **(GĐ2)** Điều hướng qua Web + vừa chạy vừa cân bằng.
> Bao gồm: quy trình tune PID, hạ tầng mô phỏng/đo lường, giao thức test, an toàn và truy vết (§7–§10).
> **Không bao gồm** bước viết báo cáo/slide/kịch bản demo (sẽ làm riêng, ngoài phạm vi roadmap này).
>
> **Tài liệu này là bản roadmap duy nhất**, đã tổng hợp và thay thế cho bộ 4 tài liệu roadmap cũ
> (2 file lộ trình/giai đoạn và 2 file kế hoạch tiến độ — đã xóa khỏi `Docs/`).
> Các tài liệu nền tảng về phần cứng/thuật toán (`Phan_Cung_Va_Ket_Noi.md`,
> `Thuat_Toan_Dieu_Khien.md`, `Huong_Dan_Di_Day_Toan_Bo_He_Thong.md`) vẫn giữ nguyên làm tham chiếu.

---

## 0. BỐI CẢNH XUẤT PHÁT (ĐÃ XÁC NHẬN)

Đồ án hiện đang ở trạng thái: **phần mềm cơ sở đã thiết lập xong + xe đã lắp ghép xong.**
Đây là **điểm xuất phát** của cả hai giai đoạn — **không phải nội dung phải làm lại**.
Toàn bộ roadmap dưới đây là phần **còn thiếu** để xe *hoạt động tốt*, không phải phần *khung sườn*.

### 0.1. Tài sản phần mềm đã có

| Nhóm          | Module                            | File                                                              | Trạng thái | Ghi chú                                                    |
| :------------- | :-------------------------------- | :---------------------------------------------------------------- | :----------: | :---------------------------------------------------------- |
| Ngoại vi      | CubeMX (clock/timer/I2C/UART DMA) | `Core/Src/tim.c`, `i2c.c`, `usart.c`, `dma.c`, `gpio.c` |      ✅      | 0 errors, 0 warnings                                        |
| Chấp hành    | Động cơ Dual A4950             | `Core/Src/motor.c`                                              |      ✅      | Slow decay, clamp ±2499, self-test                         |
| Phản hồi     | Encoder Hall 2 bánh              | `Core/Src/encoder.c`                                            |      ✅      | 1320 xung/vòng, LPF                                        |
| Cảm biến     | Bosch BMI160                      | `Core/Src/bmi160.c`                                             |      ✅      | Burst 12B, calibrate 500 mẫu                               |
| Thuật toán   | Lọc bù góc Pitch               | `Core/Src/filter.c`                                             |      ✅      | Complementary α = 0.98                                     |
| Thuật toán   | Cascade PID 2 vòng               | `Core/Src/pid.c`                                                |   ✅ khung   | Có sẵn trim, deadband, bù lái                           |
| Điều phối   | FSM 8 trạng thái + loop 200Hz   | `Core/Src/robot_fsm.c`, `main.c`                              |   ✅ khung   | Ngắt TIM4 5ms, có`BENCH_TEST`                           |
| Truyền thông | UART1 DMA ↔ ESP32-S3             | `Core/Src/esp32_comm.c`                                         |      ✅      | `$CMD/$PID/$TRIM/$RACE/$BENCH/$CALIB/$STOP`               |
| Chỉ thị      | Còi + LED                        | `Core/Src/buzzer_led.c`                                         |      ✅      | Non-blocking                                                |
| Gateway        | Wi-Fi SoftAP + WebSocket          | `ESP32_S3_Gateway/ESP32_S3_Gateway.ino`                         |      ✅      | AP`TWIP_RACER_S3`, :80/:81                                |
| Giao diện     | Web HUD cơ bản                  | `ESP32_S3_Gateway/index_html.h`                                 |      ✅      | Slider, TIẾN/LÙI, DỪNG, E-STOP, telemetry, tune PID/Trim |
| Dữ liệu      | Data logger + xuất TXT           | `ESP32_S3_Gateway/index_html.h`                                 |      ✅      | Gom 20Hz, xuất`.txt` kèm header PID                     |

### 0.2. Điều kiện đầu vào đã thỏa

- [X] Firmware nạp chạy, `Motor_SelfTest()` quay nhẹ 2 bánh.
- [X] BMI160 xác thực `CHIP_ID = 0xD1`, đọc ổn định.
- [X] Encoder đọc đúng ≈ 1320 xung/vòng.
- [X] Web HUD hiển thị telemetry thời gian thực; logger xuất được file.
- [X] Xe đã lắp khung 2 tầng, siết chặt, IMU dán đệm xốp chống rung.

### 0.3. Điều còn thiếu — chính là toàn bộ nội dung roadmap

- ❌ Xác thực chiều trục IMU **trên xe thật** và chốt dấu Pitch/Gyro.
- ❌ Chốt **bộ số PID thực nghiệm** (hiện là số mặc định, chưa tune).
- ❌ Chốt **Deadband**, **θ_trim**.
- ❌ **Lái/điều hướng web** (web chưa gửi được `steer`, chưa có nút RACE/CALIB).
- ❌ **Chạy thẳng / Yaw lock**, chế độ đua, bù lái thích ứng thực nghiệm.
- ❌ Lưu Flash thông số, chống kẹt bánh.

---

## 1. TỔNG QUAN HAI GIAI ĐOẠN

|                          | **GIAI ĐOẠN 1**                                          | **GIAI ĐOẠN 2**                              |
| :----------------------- | :--------------------------------------------------------------- | :--------------------------------------------------- |
| **Câu hỏi**      | *"Xe có tự đứng và tự hồi khi bị tác động không?"* | *"Xe có chạy/bẻ lái mà không ngã không?"*  |
| **Mục tiêu**     | Tự cân bằng + chống nhiễu tại chỗ                         | Điều hướng web + vừa chạy vừa cân bằng      |
| **Đầu vào**     | Nền tảng đã xong (mục 0)                                    | Đạt**GATE 1**                                |
| **Lõi thực thi** | Tune vòng Góc PD + Vòng Vận tốc giữ vị trí               | Tune Vòng Vận tốc bám tốc độ + bù lái + Yaw |
| **Trạng thái**   | Khung xong,**chưa tune & nghiệm thu**                    | Khung xong,**chưa thực thi**                 |

```
 [NỀN TẢNG ĐÃ XONG]  ──►  [GĐ1: TỰ CÂN BẰNG]  ──GATE 1──►  [GĐ2: ĐIỀU HƯỚNG WEB]
   CubeMX · Driver            Tune PD góc                 Tune PI vận tốc · lái
   · Lắp ráp · Web cơ bản     + PI giữ vị trí              + Yaw lock · đua · Flash
```

> **Nguyên tắc bất di bất dịch:** không bắt đầu GĐ2 khi chưa đạt GATE 1.
> Xe chưa đứng vững thì mọi nỗ lực "cho chạy" chỉ làm xe lật nhanh hơn.

---

## 2. GIAI ĐOẠN 1 — TỰ ĐỘNG CÂN BẰNG

### 2.1. Mục tiêu & phạm vi

Dựng xe đứng trên 2 bánh, **tự giữ thăng bằng**, và **tự hồi khi bị tác động** (đẩy/nghiêng) mà không ngã.
Trong GĐ1, xe **chưa cần chạy**: vòng vận tốc hoạt động với `v_target = 0` chỉ để **chống trôi tại chỗ**.

| Thuộc GĐ1                                               | Không thuộc GĐ1 (để GĐ2)                 |
| :-------------------------------------------------------- | :--------------------------------------------- |
| Ước lượng góc Pitch, vòng trong PD giữ góc        | Chạy tiến/lùi theo lệnh, bẻ lái vi sai   |
| Bù Deadband ma sát, bù θ_trim                         | Bù lái thích ứng, chế độ đua, Yaw hold |
| Vòng ngoài PI**giữ vị trí** (`v_target = 0`) | Điều hướng web, chart/CSV nâng cao        |
| An toàn ngã > 45°, Failsafe UART                       | Lưu Flash, chống kẹt bánh                  |

### 2.2. Kiến trúc điều khiển GĐ1

```
  Encoder v_actual ──┐   ┌──────── VÒNG NGOÀI: GIỮ VỊ TRÍ (PI) ────────┐
                     └──►│ e_v = 0 − v_actual                          │
  Gyro ω ─────────────┐  │ θ_target = clamp(Kp2·e_v + ∫Ki2·e_v·dt, ±8°)│
  θ_actual (Filter) ──┼─►└───────────────────────┬─────────────────────┘
                      │                          ▼
                      │  ┌──────── VÒNG TRONG: GIỮ GÓC (PD) ─────────┐
                      └─►│ e_θ = (θ_target + θ_trim) − θ_actual       │
                         │ PWM_base = Kp1·e_θ − Kd1·ω_gyro             │
                         └───────────────────────┬────────────────────┘
                                                 ▼
                         Bù Deadband ±250 · clamp ±2499 · Motor_SetDuty() (steer = 0)
```

> GĐ1: `steer = 0` ⇒ hai bánh nhận **cùng** một giá trị PWM (`PWM_base`).
> Xe chỉ *giữ thăng bằng tại chỗ*, không cố ý di chuyển.

### 2.3. Quy trình thực thi (đúng trình tự, không nhảy bước)

|   Bước   | Việc làm                                                                                                                                                                                                                             | Điều kiện qua bước                          |
| :---------: | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :----------------------------------------------- |
| **0** | **Xác thực IMU & chiều trục**: nghiêng xe về trước → Pitch **tăng dương**. Nếu ngược dấu: sửa dấu `gy`/thứ tự `ax,az` trong `filter.c` (**không sửa PID**). Kiểm Gyro đứng yên ≈ 0. | Pitch đúng dấu, ổn định, không nhảy rác |
| **1** | **Tune `Kp1`** (lò xo ảo): đặt `Kp2 = Ki2 = 0`; tăng `200 → 350` tới khi có lực phản kháng rõ, bắt đầu rung thì **giảm ~10%**                                                                       | Có lực chống ngã, chưa rung                 |
| **2** | **Tune `Kd1`** (giảm chấn): tăng `3.0 → 8.5` dập rung; tinh chỉnh `Kp1 ↔ Kd1` 2–3 lượt                                                                                                                           | Buông tay xe tự đứng (còn trôi)            |
| **3** | **Chốt `MOTOR_DEADBAND`**: nghiêng ~0.5° xem bánh có nhích; "trơ" thì tăng `250 → 300`, giật cục thì giảm                                                                                                      | Phản hồi tức thì, không giật               |
| **4** | **Chốt `θ_trim`**: dùng nút `±0.1° / ±0.02°` trên web gửi `$TRIM,value*` để triệt xu hướng trôi một phía                                                                                                 | Trôi một phía giảm hẳn                      |
| **5** | **Bật vòng vận tốc PI giữ vị trí** (`v_target = 0`): tăng `Kp2` (`1.0 → 2.5`) và `Ki2` (`0.05 → 0.20`) để xe tự học góc bù và đứng bất động                                                     | Đẩy nhẹ, xe tự quay về vị trí cũ         |
| **6** | **An toàn & Failsafe**: ngã > 45° cắt PWM + còi; dựng lại tự về BALANCING; rút UART 1.0s ga/lái về 0                                                                                                                 | Không rung giật, không EMERGENCY              |

### 2.4. Thông số GĐ1 & khoảng tune

| Tham số                  | Định nghĩa                       | Giá trị hiện tại |   Khoảng tune   | File                |
| :------------------------ | :---------------------------------- | :------------------: | :---------------: | :------------------ |
| `Kp1`                   | Tỉ lệ vòng góc                  |   **350.0**   |    200 → 400    | `pid.h:20`        |
| `Kd1`                   | Vi phân vòng góc                 |    **8.5**    |    3.0 → 9.0    | `pid.h:21`        |
| `Kp2`                   | Tỉ lệ vòng vận tốc             |    **2.5**    |    1.0 → 3.0    | `pid.h:22`        |
| `Ki2`                   | Tích phân vòng vận tốc         |    **0.20**    |   0.05 → 0.25   | `pid.h:23`        |
| `MAX_TILT_NORMAL`       | Góc ngả đặt tối đa (thường) |   **8.0°**   |    cố định    | `pid.h:25`        |
| `MAX_INTEGRAL_VELOCITY` | Kẹp anti-windup                    |   **10.0°**   |      xem 4.1      | `pid.h:27`        |
| `MOTOR_DEADBAND`        | Bù ma sát hộp số                |    **250**    |    200 → 300    | `motor.h:24`      |
| `θ_trim`               | Bù trọng tâm tĩnh               |   **0.00°**   | −1.0° → +1.0° | `pid.c:14`        |
| Ngưỡng kích hoạt      | Pitch để vào cân bằng          |   **15.0°**   |      xem 4.1      | `robot_fsm.c:180` |
| Ngưỡng ngã             | Pitch cắt động cơ               |   **45.0°**   |    cố định    | `robot_fsm.c:165` |

### 2.5. GATE 1 — Tiêu chí nghiệm thu GĐ1

| # | Hạng mục                    | Tiêu chuẩn đạt                                         | Phương pháp          |
| :-: | :---------------------------- | :--------------------------------------------------------- | :---------------------- |
| 1 | Tự đứng thăng bằng tĩnh | Đứng yên**> 60 s**, trôi **< 10 cm**       | Sàn phẳng + telemetry |
| 2 | Chống nhiễu                 | Đẩy 3 lần liên tiếp,**ts < 0.8 s**, không ngã | Tay đẩy nhẹ đầu xe |
| 3 | Sai số góc tĩnh            | **\|e_ss\| ≤ ± 0.3°**                             | `$TEL` ghi ra file    |
| 4 | Cắt an toàn khi ngã        | Cắt PWM**< 5 ms** khi > 45°                        | Nghiêng quá ngưỡng  |
| 5 | Failsafe mất sóng           | Sau**1.0 s** ga/lái về 0, xe vẫn tự đứng       | Ngắt Wi-Fi/UART        |
| 6 | Không lỗi tự phát         | Không`EMERGENCY`, không rung giật kéo dài           | Badge trạng thái web  |

### 2.6. Việc phần mềm cần bổ sung cho GĐ1

- [ ] **WP1.7 — Cân nhắc ngưỡng kích hoạt**: code và tài liệu hiện đang thống nhất `< 15°` (`robot_fsm.c:180`).
  Đề xuất cân nhắc siết về **`< 8–10°`** (dễ kích hoạt nhưng chưa ngã ngang) rồi đồng bộ code + tài liệu.
- [ ] **WP1.8 (tùy chọn) — Nút `$CALIB` trên web**: STM32 đã parse (`esp32_comm.c:97`), web chưa có nút gửi.
- [ ] **WP1.9 (tùy chọn) — Lưu θ_trim + PID vào Flash**: tránh mất tham số sau khi tắt nguồn.
- [ ] **WP1.10 — Đo lường & timestamp phần cứng** (xem §7.2): thêm `loop_count` vào `$TEL`, đo jitter/latency, nâng tần số telemetry khi tune.
- [ ] **WP1.11 — An toàn phần cứng** (xem §9.3): watchdog độc lập `IWDG`, đọc điện áp pin thật qua ADC, cắt khi pin yếu, xử lý brown-out.
  *(Lưu ý: `.ioc` hiện **chưa cấu hình ADC**; `batt_voltage` trong `robot_fsm.c` đang hard-code `12.0f` → số "Pin" trên web là giả. Cần thêm cầu phân áp + ADC.)*

### 2.7. Rủi ro GĐ1 & cách xử lý

| Rủi ro                       | Biểu hiện                | Cách xử lý                                          |
| :---------------------------- | :------------------------- | :----------------------------------------------------- |
| Nhiễu rung cơ khí vào IMU | Pitch nhảy rác, xe rung  | Đệm xốp 2–3 mm; tăng IC filter; tăng α lọc bù |
| Ground bounce từ Driver      | Gyro lệch khi motor chạy | GND Driver về WAGO2, Star GND                         |
| Trọng tâm lệch nặng       | Xe luôn lao một phía    | Bù`θ_trim`; dồn pin sát trục                    |
| Deadband quá lớn            | Xe giật cục              | Giảm`MOTOR_DEADBAND`                                |
| `Kd1` quá cao              | Xe "ì", đáp ứng chậm  | Giảm`Kd1`, tăng nhẹ `Kp1`                       |

---

## 3. GIAI ĐOẠN 2 — ĐIỀU HƯỚNG WEB + VỪA CHẠY VỪA CÂN BẰNG

### 3.1. Mục tiêu & phạm vi

Điều khiển xe từ **Web HUD** để **tiến / lùi / bẻ lái**, trong khi xe **vẫn giữ thăng bằng động**.
Đây là **nâng cấp của GĐ1** — kế thừa nguyên vẹn vòng trong PD, lọc bù, deadband, θ_trim, an toàn, FSM.

### 3.2. Kiến trúc điều khiển GĐ2

```
  Web HUD ──ws──► ESP32-S3 ──UART1──► STM32
    ▲                ▲                  │
    │  $TEL 20Hz     │                  ▼
    └────────────────┘   ┌──────── VÒNG NGOÀI: BÁM VẬN TỐC (PI) ────────┐
                         │ e_v = v_target − v_actual                    │
                         │ θ_target = clamp(Kp2·e_v + ∫Ki2·e_v·dt,     │
                         │            ±8° thường / ±15° đua)           │
                         └───────────────────────┬─────────────────────┘
                                                 ▼
                         ┌──────── VÒNG TRONG: GÓC (PD) ───────────────┐
                         │ e_θ = (θ_target + θ_trim) − θ_actual        │
                         │ PWM_base = Kp1·e_θ − Kd1·ω_gyro             │
                         └───────────────────────┬─────────────────────┘
  Lệnh lái ─► steer_eff = steer / (1 + β·|v_actual|)   (β = 1.2)
                         ┌───────────────────────▼─────────────────────┐
                         │ PWM_left = PWM_base + steer_eff             │
                         │ PWM_right = PWM_base − steer_eff            │
                         └───────────────────────┬─────────────────────┘
                                                 ▼
                         Bù Deadband · clamp ±2499 · Motor_SetDuty()
```

**Khác biệt cốt lõi so với GĐ1:** `v_target ≠ 0`, hai bánh nhận **PWM khác nhau**, và vòng vận tốc
phải **chủ động nghiêng xe** để tạo gia tốc — theo cơ chế `tanθ ≈ a_x / g`.

### 3.3. Giao thức lệnh (STM32 đã implement)

| Gói lệnh                |    Chiều    | Ý nghĩa                     | Xử lý              |         Web gửi?         |
| :------------------------ | :----------: | :---------------------------- | :------------------- | :------------------------: |
| `$CMD,v,steer*`         | Web → STM32 | Đặt vận tốc + lái        | `esp32_comm.c:72`  | ✅ (20Hz, steer hiện = 0) |
| `$PID,kp1,kd1,kp2,ki2*` | Web → STM32 | Cập nhật PID                | `esp32_comm.c:82`  |             ✅             |
| `$TRIM,value*`          | Web → STM32 | Bù trọng tâm               | `esp32_comm.c:89`  |             ✅             |
| `$RACE,1/0*`            | Web → STM32 | Bật/tắt chế độ đua      | `esp32_comm.c:92`  |  ❌**thiếu nút**  |
| `$BENCH,1/0*`           | Web → STM32 | Bật/tắt chế độ Test Bàn | `esp32_comm.c:94`  |    ✅ (nút Test Bàn)    |
| `$CALIB*`               | Web → STM32 | Hiệu chuẩn lại IMU         | `esp32_comm.c:97`  |  ❌**thiếu nút**  |
| `$STOP*`                | Web → STM32 | Dừng khẩn cấp              | `esp32_comm.c:99`  |             ✅             |
| `$TEL,...\r\n`          | STM32 → Web | Telemetry 20Hz (10 trường)  | `esp32_comm.c:186` |         ✅ (nhận)         |

### 3.4. Quy trình thực thi

|   Bước   | Việc làm                                                                                                                                                                                                                | Điều kiện qua bước                        |
| :---------: | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :--------------------------------------------- |
| **1** | **Tune vòng vận tốc bám tốc độ** (`steer = 0`): gạt slider + giữ TIẾN, tinh chỉnh `Kp2` (bám nhanh, không dao động) và `Ki2` (khử sai số tĩnh)                                             | `v_actual ≈ v_target`, không vọt lố lớn |
| **2** | **Bổ sung joystick/nút lái + tune bù lái** (`β`): β nhỏ → lái mạnh dễ lật; β lớn → lái yếu. Mục tiêu: xoay tròn tốt khi đứng yên, vào cua êm khi nhanh                                 | Vào cua không lật, không trượt bánh     |
| **3** | **Chạy thẳng / Yaw lock**: bù `k_left/k_right` trong `Motor_SetDuty()`, hoặc khóa hướng bằng Gyro Yaw `G_z` khi `steer = 0`                                                                         | Đi thẳng tắp không nhao lái               |
| **4** | **Chế độ đua**: thêm nút `$RACE,1*` → mở `MAX_TILT` lên ±15° (gia tốc ≈ 2.6 m/s²); kiểm bứt tốc/phanh/cua                                                                                      | `v ≥ 1.8 m/s`, không ngã                  |
| **5** | **Nâng cấp Web HUD**: nút RACE/CALIB, joystick lái, chart realtime, preset Smooth/Race                                                                                                                          | Điều khiển & quan sát đầy đủ           |
| **6** | **Lưu Flash + chống kẹt bánh**: thêm parse `$SAVE*` (hiện STM32 **chưa** xử lý) để ghi `Kp1,Kd1,Kp2,Ki2,θ_trim` vào Sector 7; nếu `PWM > 80%` quá 500 ms mà `v ≈ 0` ⇒ `EMERGENCY` | Tham số bền vững; bảo vệ motor            |

### 3.5. Thông số GĐ2 & khoảng tune

| Tham số                   | Định nghĩa                         |    Giá trị hiện tại    |    Khoảng tune    | File                 |
| :------------------------- | :------------------------------------ | :------------------------: | :----------------: | :------------------- |
| `Kp2`                    | Tỉ lệ vòng vận tốc               |       **2.5**       |     1.0 → 3.0     | `pid.h:22`         |
| `Ki2`                    | Tích phân vòng vận tốc           |       **0.20**       |    0.05 → 0.25    | `pid.h:23`         |
| `β` (beta)              | Hệ số suy giảm lái theo vận tốc |       **1.2**       |     0.5 → 2.0     | `pid.c:85`         |
| `MAX_TILT_NORMAL / RACE` | Giới hạn góc ngả                  |    **8° / 15°**    | 8° / 12° → 18° | `pid.h:25-26`      |
| Vận tốc tối đa web     | Slider                                | **0.10 → 1.50 m/s** |   theo thực tế   | `index_html.h:187` |
| Failsafe                   | Mất lệnh UART                       |     **1000 ms**     |   500 → 1500 ms   | `esp32_comm.c:166` |

> `Kp1`, `Kd1`, `Deadband`, `θ_trim` **kế thừa GĐ1** — chỉ chỉnh lại nếu việc chạy làm đổi điểm cân bằng.

### 3.6. GATE 2 — Tiêu chí nghiệm thu GĐ2

| # | Hạng mục                 | Tiêu chuẩn đạt                                            | Phương pháp                       |
| :-: | :------------------------- | :------------------------------------------------------------ | :----------------------------------- |
| 1 | Điều hướng cơ bản    | Tiến/lùi mượt theo`v_target` 0 → 1.50 m/s (max slider) | Slider + giữ TIẾN/LÙI             |
| 2 | Vừa chạy vừa cân bằng | Chạy 10 m liên tục không ngã                             | Quan sát + telemetry                |
| 3 | Bẻ lái linh hoạt        | Bo cua bán kính**< 0.4 m**, không lật               | Lái zíc-zắc qua 5 chốt cách 1 m |
| 4 | Vận tốc tối đa (đua)  | **≥ 1.8 m/s**                                          | Đo bằng xung Encoder               |
| 5 | Chạy thẳng               | Lệch ngang nhỏ khi`steer = 0`                             | Chạy trên đường kẻ             |
| 6 | An toàn                   | Ngã > 45° cắt PWM < 5 ms; mất sóng 1 s về 0             | Test có chủ đích                 |
| 7 | Số liệu                  | Xuất được file telemetry, đồ thị đáp ứng hợp lệ   | Web logger                           |

### 3.7. Việc phần mềm cần bổ sung cho GĐ2

- [ ] **WP2.1 — Nút lái trên web**: hiện `$CMD` luôn gửi `steer = 0` (`index_html.h:362`); thêm joystick/nút gửi `steer`.
- [ ] **WP2.2 — Nút RACE & CALIB**: gửi `$RACE,1*` / `$RACE,0*` và `$CALIB*` (STM32 đã sẵn sàng; riêng nút `$BENCH` đã có sẵn trên web).
- [ ] **WP2.3 — Yaw lock / bù cân bằng động 2 motor**: thêm bù vi sai `ΔPWM_yaw = −K_yaw·G_z` khi `steer = 0`.
- [ ] **WP2.4 — Chart realtime**: vẽ `θ_target`, `θ_actual`, `v_act` bằng Canvas trên web.
- [ ] **WP2.5 — Lưu Flash thông số** (`flash_storage.c/.h`, Sector 7): nhận `$SAVE*`, tự load khi khởi động.
- [ ] **WP2.6 — Chống kẹt/trượt bánh (stall)**: cắt động cơ về `EMERGENCY` + còi.

> **Đã xong:** xuất file telemetry `.txt` (task 4.6 cũ). Có thể nâng lên CSV nếu muốn tiện phân tích.

### 3.8. Rủi ro GĐ2 & cách xử lý

| Rủi ro                      | Biểu hiện            | Cách xử lý                                 |
| :--------------------------- | :--------------------- | :-------------------------------------------- |
| `Kp2`/`Ki2` quá lớn    | Xe vọt lố, lắc dọc | Giảm`Kp2`, giảm `Ki2`, giữ anti-windup |
| β quá nhỏ khi chạy nhanh | Vào cua bay xe        | Tăng β, giảm`MAX_TILT`                   |
| Nhao lái                    | Xe xoay khi đi thẳng | Yaw lock / bù`k_left, k_right`             |
| Sụt áp khi chạy nhanh     | ESP32 reset            | Tụ hóa 1000µF, tách nguồn động lực    |
| Mất Wi-Fi khi đang chạy   | Mất điều khiển     | Failsafe 1 s về 0, xe vẫn tự đứng        |
| Kẹt chướng ngại          | Motor ăn dòng, nóng | Stall detection cắt động cơ               |

---

## 4. ĐỒNG BỘ TÀI LIỆU & CODE (CẦN THỐNG NHẤT TRƯỚC KHI NGHIỆM THU)

|  #  | Hạng mục                          | Code hiện tại                                           | Tài liệu/kế hoạch                       | Đề xuất                                              |
| :-: | :---------------------------------- | :-------------------------------------------------------- | :------------------------------------------ | :------------------------------------------------------ |
| 4.1 | Ngưỡng tự kích hoạt cân bằng | `< 15.0°` (`robot_fsm.c:180`)                        | `< 15.0°` (`Thuat_Toan_Dieu_Khien.md`) | Đã khớp; cân nhắc siết về**`< 8–10°`** |
| 4.2 | Kẹp anti-windup`Ki2`             | `±10.0°` (`pid.h:27`)                               | `±10°` (`Thuat_Toan_Dieu_Khien.md`)   | Đã khớp                                              |
| 4.3 | Hệ số lọc vận tốc Encoder      | `β = 0.75` (`encoder.h:27`)                          | Khuyến nghị`β ≈ 0.65–0.75`           | Đang khớp; thử`0.70` nếu cần                     |
| 4.4 | Hàm test động cơ                | `Motor_SelfTest()` + `Motor_Test_Run()` (`motor.c`) | (không còn xung đột)                    | Đã bổ sung`Motor_Test_Run()`                       |

---

## 5. THỨ TỰ ƯU TIÊN & DANH SÁCH WORK PACKAGE

### 5.1. Thứ tự ưu tiên tổng thể

```
 [NỀN] WP0 Mô hình & Mô phỏng vòng kín (pre-tune, khuyến nghị)  ─────────────┐
                                                                            ▼
 GĐ1: WP1.0 Xác thực IMU ─► WP1.1 Tune Kp1 ─► WP1.2 Tune Kd1 ─► WP1.3 Deadband
      ─► WP1.4 θ_trim ─► WP1.5 Tune Kp2/Ki2 (giữ vị trí) ─► WP1.6 An toàn SW
      ─► WP1.7 Ngưỡng ─► WP1.10 Đo lường/Timestamp ─► WP1.11 An toàn HW   ══► GATE 1
 GĐ2: WP2.0 Tune vòng vận tốc (tách băng thông) ─► WP2.1 Lái web ─► WP2.3 Yaw lock
      ─► WP2.4 Chế độ đua ─► WP2.5 Web nâng cao ─► WP2.6 Flash + Stall
      ─► WP2.7 Kiểm bền & Gain scheduling                                 ══► GATE 2
```

### 5.2. Bảng work package (có điều kiện vào / ra)

| WP     | Tên                                                                                                | GĐ | Điều kiện vào (Entry)                | File chạm tới                         | Điều kiện ra (Exit/DoD)                         |
| :----- | :-------------------------------------------------------------------------------------------------- | :--: | :--------------------------------------- | :-------------------------------------- | :------------------------------------------------- |
| WP0    | Mô hình hóa & mô phỏng vòng kín                                                              | Nền | Có tham số cơ khí (M, L, R, ma sát) | `sim/` (Python/Simulink)              | Bộ gain gợi ý + đồ thị mô phỏng            |
| WP1.0  | Xác thực IMU & chiều trục                                                                       | GĐ1 | Firmware chạy, telemetry sống          | `filter.c`, `bmi160.c`              | Pitch đúng dấu, ổn định, không nhiễu rác  |
| WP1.1  | Tune`Kp1`                                                                                         | GĐ1 | WP1.0 xong;`Kp2=Ki2=0`                 | `pid.h`/web                           | Có lực chống ngã, chưa rung                   |
| WP1.2  | Tune`Kd1`                                                                                         | GĐ1 | WP1.1 xong                               | `pid.h`/web                           | Buông tay xe tự đứng (còn trôi)              |
| WP1.3  | Chốt Deadband                                                                                      | GĐ1 | WP1.2 xong                               | `motor.h`                             | Phản hồi tức thì, không giật                 |
| WP1.4  | Chốt θ_trim                                                                                       | GĐ1 | WP1.3 xong                               | `pid.c`/web                           | Trôi một phía giảm hẳn                        |
| WP1.5  | Tune`Kp2/Ki2` giữ vị trí                                                                       | GĐ1 | WP1.4 xong                               | `pid.h`/web                           | Đứng bất động, đẩy tự hồi                 |
| WP1.6  | An toàn SW & Failsafe                                                                              | GĐ1 | Firmware ổn định                      | `robot_fsm.c`, `esp32_comm.c`       | Ngã cắt < 5 ms; mất sóng 1 s về 0             |
| WP1.7  | Chốt ngưỡng kích hoạt                                                                          | GĐ1 | WP1.6 xong                               | `robot_fsm.c`                         | Code + tài liệu khớp                            |
| WP1.8  | Nút`$CALIB` trên web *(tùy chọn)* | GĐ1 | — | `index_html.h` | Gửi được `$CALIB*` |      |                                          |                                         |                                                    |
| WP1.9  | Lưu θ_trim + PID vào Flash*(tùy chọn)*                                                       | GĐ1 | —                                       | `flash_storage.*`                     | Tắt/bật nguồn không mất tham số              |
| WP1.10 | Đo lường & timestamp phần cứng                                                                 | GĐ1 | WP1.0 xong                               | `esp32_comm.c`, `main.c`, web       | Có mốc thời gian HW; jitter/latency đo được |
| WP1.11 | An toàn phần cứng                                                                                | GĐ1 | Có cầu phân áp + ADC                 | `main.c`, `adc.c`, `robot_fsm.c`  | IWDG chạy; cắt khi pin yếu/brown-out            |
| WP2.0  | Tune vòng vận tốc bám tốc độ                                                                 | GĐ2 | GATE 1 đạt                             | `pid.h`/web                           | `v_actual ≈ v_target`, tách băng thông       |
| WP2.1  | Nút/joystick lái trên web                                                                        | GĐ2 | WP2.0 xong                               | `index_html.h`                        | Gửi được`steer ≠ 0`                         |
| WP2.2  | Nút RACE/CALIB                                                                                     | GĐ2 | —                                       | `index_html.h`                        | Gửi`$RACE,*` / `$CALIB*`                      |
| WP2.3  | Yaw lock / cân bằng động                                                                        | GĐ2 | WP2.1 xong                               | `pid.c`, `robot_fsm.c`, `motor.c` | Đi thẳng không nhao                             |
| WP2.4  | Chế độ đua ±15°                                                                               | GĐ2 | WP2.3 xong                               | `pid.h`, web                          | `v ≥ 1.8 m/s`, không ngã                      |
| WP2.5  | Web nâng cao (chart realtime)                                                                      | GĐ2 | —                                       | `index_html.h`                        | Đồ thị realtime chạy ổn                       |
| WP2.6  | Lưu Flash + Stall protection                                                                       | GĐ2 | —                                       | `flash_storage.*`, `robot_fsm.c`    | Tham số bền vững; cắt khi kẹt                 |
| WP2.7  | Kiểm bền & Gain scheduling                                                                        | GĐ2 | WP2.4 xong                               | `pid.c`/web                           | Ổn định trên ≥3 mặt sàn & mức pin          |

---

## 6. BẢNG THEO DÕI & CHỐT THÔNG SỐ (ĐIỀN SAU KHI TUNE)

### Bảng A — Giai đoạn 1

|     Kp1     |     Kd1     |     Kp2     |     Ki2     |   Deadband   |   θ_trim   | Ngày | Người tune |
| :----------: | :----------: | :----------: | :----------: | :----------: | :----------: | :---: | :----------- |
| _(trống)_ | _(trống)_ | _(trống)_ | _(trống)_ | _(trống)_ | _(trống)_ |      |              |

### Bảng B — Giai đoạn 2

|     Kp2     |     Ki2     |      β      |    K_yaw    | MAX_TILT (thường/đua) | Ngày | Người tune |
| :----------: | :----------: | :----------: | :----------: | :----------------------: | :---: | :----------- |
| _(trống)_ | _(trống)_ | _(trống)_ | _(trống)_ |        8° / 15°        |      |              |

---

## 7. HẠ TẦNG PHỤC VỤ TUNING PID

> Đây là các hạng mục **bắt buộc/tùy chọn** để việc tune PID *đo được, lặp lại được và an toàn* —
> thứ quyết định xe có thực hiện đúng chức năng hay không. Tham chiếu tốt nghiệp ở mục 7.1 và 7.2.

### 7.1. Mô hình hóa & mô phỏng vòng kín (WP0 — khuyến nghị mạnh)

Tài liệu học thuật (MBSE inverted pendulum, V-REP benchmark, sim2real) đều cho thấy **mô phỏng trước khi
chạy phần cứng** giúp pre-tune gain và giảm số lần xe ngã/hỏng.

* **Việc làm**:
  1. Xây mô hình con lắc ngược từ tham số thật: `M, L, R, g`, ma sát hộp số, đặc tính Encoder.
  2. Mô phỏng **rời rạc 200Hz** đúng cấu trúc cascade (vòng trong PD + vòng ngoài PI, deadband, trim).
  3. Quét gain để tìm vùng ổn định; tính `Mp, ts, ess` trên mô hình.
* **Đầu ra**: bộ `(Kp1, Kd1, Kp2, Ki2)` gợi ý + đồ thị đáp ứng → dùng làm **điểm khởi đầu** khi tune xe thật (thay vì bắt đầu từ 0).
* **Công cụ**: Python (`numpy`/`scipy`/`control`) hoặc MATLAB/Simulink. Không cần real-time.

### 7.2. Kỷ luật đo lường & thời gian thực (WP1.10)

Nghiên cứu "Digital Control of an Inverted Pendulum" (MDPI) chỉ rõ **dead-time/latency của điều khiển số
làm giảm vùng ổn định** — nên phải đo được, không chỉ suy đoán.

* **Đo các đại lượng sau và ghi lại**:
  * Jitter ngắt `TIM4` (chu kỳ thực tế so với 5.0ms) — dùng chân debug bật/tắt trong ngắt.
  * Thời gian `BMI160_Read_All()` (kỳ vọng < 350µs) và tổng CPU load vòng 200Hz.
  * Latency đường đo: STM32 → UART → ESP32 → Web (để không nhầm với động học xe).
* **Thay đổi code cần làm**:
  * **Thêm `loop_count` (× 5ms) vào gói `$TEL`** để có **timestamp phần cứng**, loại bỏ jitter do
    timestamp trình duyệt (`Date.now()`).
  * **Nâng tần số telemetry khi tune** từ 20Hz lên **50–100Hz** (đủ bắt dao động vòng góc).
    *Ước lượng băng thông*: gói ~65 byte × 20Hz ≈ 1.3 kB/s; UART 115200 ≈ 11.5 kB/s → 100Hz ≈ 6.5 kB/s vẫn khả thi.
* **Giới hạn cần nhớ**: telemetry 20Hz hiện tại (Nyquist 10Hz) **không đủ** để thấy rung cao tần >10Hz.

### 7.3. Tách băng thông 2 vòng (cascade bandwidth separation)

Nguyên lý cascade: **vòng ngoài phải chậm hơn vòng trong ~5–10 lần** để hai vòng không "đánh nhau".

* Vòng góc (inner) giữ 200Hz.
* Vòng vận tốc (outer): thiết kế **băng thông thấp hơn** — có thể cập nhật **decimate 20–50Hz**
  (chạy PID vận tốc mỗi 4–10 chu kỳ 200Hz) thay vì mỗi chu kỳ.
* Khi tune `Kp2/Ki2` ở GĐ2, nếu thấy xe lắc chậm 0.5–1.5Hz (say rượu) → giảm `Kp2` và/hoặc tăng decimation.

### 7.4. Version hóa & baseline (bắt buộc để tune lặp lại được)

* Mỗi lần tune phải **chốt được bộ số + phiên bản firmware** để tái lập:
  * Ghi thêm vào header file log: `firmware_version`, `git_commit` (hiện chỉ có PID/Trim).
  * Đặt **tag Git** cho baseline sau mỗi GATE.
  * Lưu bộ số đã chốt vào Bảng A/B (§6) và (nếu có) vào Flash (§WP1.9/WP2.5).

---

## 8. GIAO THỨC TEST & NGHIỆM THU KỸ THUẬT

> Mọi kết luận về chất lượng điều khiển phải dựa trên **số liệu log**, không cảm quan.

### 8.1. Test vòng góc (inner) — có đai/tether nếu cần

* Đặt `Kp2 = Ki2 = 0`; tắt deadband; giữ xe an toàn.
* **Test đáp ứng bước góc**: nghiêng xe **10° rồi thả** → mục tiêu **hồi phục < 1s**, ít vọt lố.
* Ghi log 100Hz (WP1.10) → tính `Mp, ts`.

### 8.2. Test vòng vận tốc (outer)

* Đặt `v_target = 0` → xe đứng yên; sau đó **bước `v_target = 0.5 m/s`** → đo thời gian bám và độ vọt lố.
* Đo sai số tĩnh `ess` ở `v_target = 0.3 / 0.6 / 1.0 m/s`.

### 8.3. Test chống nhiễu

* Đẩy thân xe 3 lần liên tiếp → mục tiêu `ts < 0.8s`, không ngã (GATE 1).

### 8.4. Test độ bền (robustness) — WP2.7

* Chạy lại bộ số trên **≥ 3 mặt sàn khác nhau** (gạch, gỗ, thảm), **mức pin khác nhau**, **tải khác**.
* Nếu lệch nhiều khi nghiêng > ±5° → xem xét **gain scheduling** (bảng gain theo dải góc).

### 8.5. Test an toàn (liên kết §9)

* Ngã > 45°; mất sóng > 1s; pin yếu; watchdog; kẹt bánh — mỗi cái một kịch bản có chủ đích.

### 8.6. Chỉ số chất lượng điều khiển dùng để chấm

| Chỉ số       | Ý nghĩa                                    | Ngưỡng mục tiêu   |
| :------------- | :------------------------------------------- | :-------------------- |
| `Mp`         | Độ vọt lố                                | < 15%                 |
| `ts`         | Thời gian xác lập                         | < 0.8 s               |
| `ess`        | Sai số góc tĩnh                           | ≤ ± 0.3°           |
| `IAE / ITAE` | Tích phân sai số (so sánh các bộ gain) | càng nhỏ càng tốt |

---

## 9. AN TOÀN & ĐỘ BỀN HỆ THỐNG (MỞ RỘNG)

### 9.1. An toàn phần mềm (đã có)

* Cắt PWM khi ngã > 45°; tự kích hoạt lại khi dựng thẳng.
* Failsafe mất UART 1.0s → ga/lái về 0.
* `EMERGENCY` khi mất I2C BMI160.

### 9.2. Bảo vệ vận hành (bổ sung)

* Giới hạn dòng/nhiệt động cơ; cảnh báo khi PWM cao kéo dài mà không chuyển động (stall – WP2.6).

### 9.3. An toàn phần cứng (WP1.11 — bổ sung)

| Hạng mục                                 | Rủi ro nếu thiếu                    | Việc làm                                                 |
| :----------------------------------------- | :------------------------------------- | :--------------------------------------------------------- |
| **Watchdog độc lập `IWDG`**     | Firmware treo → xe chạy mãi         | Bật IWDG, feed trong vòng lặp chính                    |
| **Đọc điện áp pin thật (ADC)** | `batt_voltage` đang hard-code 12.0V | Cầu phân áp + ADC; cắt khi pin < ngưỡng (vd 10.5V)   |
| **Brown-out / reset nguồn**         | Sụt áp → MCU reset giữa lúc chạy | BOD + tụ nguồn; phát hiện reset bất thường          |
| **E-stop**                           | Hiện chỉ là lệnh phần mềm        | (Nếu được) công tắc ngắt cứng đường cấp Driver |

> ⚠️ **Số "Pin" trên web hiện là giá trị giả (hằng số 12.0V)** vì `.ioc` chưa có ADC. Cần phần cứng mới
> (cầu phân áp về chân ADC) — nằm ngoài phạm vi phần mềm thuần.

---

## 10. QUẢN TRỊ DỰ ÁN: TRUY VẾT, FMEA & LỊCH TRÌNH

### 10.1. Ma trận truy vết yêu cầu (RTM nhẹ)

| Yêu cầu                       | Tiêu chí (GATE) | WP thực hiện | Cách kiểm chứng |
| :------------------------------ | :---------------- | :------------- | :----------------- |
| Tự đứng cân bằng           | GATE 1#1          | WP1.0–WP1.5   | Test 60s + log     |
| Chống nhiễu                   | GATE 1#2          | WP1.2, WP1.5   | Đẩy 3 lần       |
| Sai số tĩnh                   | GATE 1#3          | WP1.4, WP1.5   | Log`$TEL`        |
| An toàn khi ngã               | GATE 1#4          | WP1.6, WP1.11  | Nghiêng > 45°    |
| Điều hướng & bám tốc độ | GATE 2#1,2        | WP2.0, WP2.1   | Slider + log       |
| Bẻ lái không lật            | GATE 2#3          | WP2.1, WP2.3   | Sa bàn zíc-zắc  |
| Tốc độ đua                  | GATE 2#4          | WP2.4          | Xung Encoder       |

### 10.2. FMEA rút gọn

| Chức năng        | Kiểu lỗi        | Ảnh hưởng        | Phát hiện             | Xử lý                   |
| :----------------- | :---------------- | :------------------ | :---------------------- | :------------------------ |
| Ước lượng góc | Gyro trôi/nhiễu | Xe lắc, ngã       | Log pitch vs thời gian | Calib, tăng α lọc bù  |
| Cấp PWM           | Chết 1 kênh     | Xe xoay tại chỗ   | Self-test, log PWM L/R  | Đảo dây/kiểm Driver   |
| Đọc Encoder      | Mất xung         | Vòng vận tốc sai | So`v_l` vs `v_r`    | Kiểm dây/xoắn đôi    |
| Nguồn             | Sụt áp đỉnh   | ESP32 reset         | Log mất gói           | Tụ 1000µF, tách nguồn |
| Kết nối          | Mất Wi-Fi        | Mất điều khiển  | Badge "mất tín hiệu" | Failsafe 1s               |

### 10.3. Lịch trình & phụ thuộc (ước lượng — điều chỉnh theo thực tế)

| WP                                   | Ước lượng (buổi) | Phụ thuộc | Ghi chú                                 |
| :----------------------------------- | :-------------------: | :---------- | :--------------------------------------- |
| WP0 Mô phỏng                       |         1–2         | —          | Có thể chạy song song                 |
| WP1.0 Xác thực IMU                 |          0.5          | —          |                                          |
| WP1.1–WP1.2 Tune Kp1/Kd1            |         2–4         | WP1.0       | **Nút thắt – lặp nhiều lần** |
| WP1.3–WP1.4 Deadband/Trim           |           1           | WP1.2       |                                          |
| WP1.5 Tune vòng vận tốc           |         1–2         | WP1.4       |                                          |
| WP1.6–WP1.7 An toàn/ngưỡng       |           1           | WP1.5       | ══►**GATE 1**                   |
| WP1.10–WP1.11 Đo lường/HW safety |         1–2         | WP1.5       | Nên làm sớm để hỗ trợ tune        |
| WP2.0 Tune vòng vận tốc (chạy)   |         1–2         | GATE 1      |                                          |
| WP2.1–WP2.3 Lái/Yaw                |         2–3         | WP2.0       |                                          |
| WP2.4 Đua                           |           1           | WP2.3       |                                          |
| WP2.5–WP2.6 Web/Flash/Stall         |           2           | —          | Có thể song song                       |
| WP2.7 Kiểm bền                     |         1–2         | WP2.4       | ══►**GATE 2**                   |

> **Đường găng (critical path)**: WP1.0 → WP1.1 → WP1.2 → WP1.5 → **GATE 1** → WP2.0 → WP2.3 → WP2.4 → **GATE 2**.
> Hai WP dài/lặp nhất là **tune vòng góc** và **tune vòng vận tốc bám tốc độ**.

### 10.4. Vòng lặp bắt buộc

Việc tune là **lặp**, không tuyến tính. Phải **re-validate** sau:

1. Mỗi khi thay đổi cơ khí (dồn pin, đổi đệm xốp, siết lại trục bánh) → chạy lại WP1.0–WP1.4.
2. Sau khi bật vòng vận tốc (WP1.5) → kiểm lại vòng góc không bị suy giảm.
3. Sau khi bật chế độ đua (WP2.4) → kiểm lại `β`, `Kp2`.
