# Mọi trường hợp firmware tự cắt motor — RT29-NOCUT

Tài liệu này liệt kê **tất cả** đường dẫn tới việc motor bị ngắt trong firmware đang nạp
(`BAL-20261115-R12-8DIR-RT29-NOCUT`). Số dòng đúng theo mã nguồn tại commit phát hành.

## 0. Cơ chế chung

Mọi lần cắt có ghi lỗi đều đi qua **một** hàm `BalanceStop(reason)`
(`Xe_Can_Bang/Core/Src/robot_fsm.c:208`):

1. `s_robot.fault |= reason` — các bit **OR tích luỹ** từ lần ARM gần nhất (xoá ở dòng 300);
2. `s_robot.state = ROBOT_STATE_FALLEN` (5);
3. `Motor_Stop()` — phanh cả hai bánh (cả 4 chân IN = 1, slow decay, `motor.c:79`);
4. xoá `BalanceMotion` / `BalanceTurn` / `BalanceRecovery` / `bal_interlock`, xoá lệnh MOVE8 đang giữ;
5. `bal_still = 0` → phải đứng yên lại 200 ms mới ARM được.

Không có tự ARM lại. Sau khi cắt, mỗi tick vòng 200 Hz lại gọi `Motor_Stop()` ở nhánh `else`
(`robot_fsm.c:394`) cho tới khi người vận hành ARM lại.

## 1. Các fault tự cắt motor

| Mã | Điều kiện | Ngưỡng | Nơi phát |
|---|---|---|---|
| **1** | Lệnh STOP từ người vận hành (`$STOP*` → `cmd->emergency_stop`) | tức thời | `robot_fsm.c:272` ← `esp32_comm.c:117` |
| **2** | Không nhận packet nào từ ESP32 | > 10 000 ms (`BALANCE_HEARTBEAT_MS`) | `robot_fsm.c:305` |
| **4** | `\|pitch\|` vượt giới hạn nghiêng | 25° thường; capture: max(25, \|θ_ARM\|+5) ≤ 75°; REC1: max(25, 50°) | `robot_fsm.c:350`, `balance_recovery.h:97` |
| **8** | (a) `\|x − cancel_origin\|` quá xa mốc phanh | 1,25 m khi đang phanh; 1,0 m khi đã huỷ | `balance_motion.h:64` |
| **8** | (b) `\|x − x_ref\|` vượt budget | 0,30–1,0 m theo góc ARM; REC1: max(budget, \|x_lúc_vào\|+0,80) ≤ 1,0 m | `robot_fsm.c:351`, `balance_startup.h:15`, `balance_recovery.h:78` |
| **16** | `\|v\|` vượt trần tốc độ | ngay khi > 1,45 m/s; hoặc 200 ms (40 tick) > max(envelope, \|v_ref\|+0,12); MAX80: `\|v_bánh\|` > 1,20 | `robot_fsm.c:364`, `balance_interlock.h:26-39` |
| **32** | `BMI160_Read_All()` fail một lần | tức thời, vào `EMERGENCY`; phải ĐO LẠI GYRO mới dùng lại được | `robot_fsm.c:254`; lần hai khi re-init fail: `robot_fsm.c:162` |
| **64** | (a) chu kỳ vòng lặp ngoài cửa sổ | < 4 000 µs hoặc > 6 000 µs | `robot_fsm.c:367` |
| **64** | (b) thời gian chạy một tick | > 4 000 µs | `robot_fsm.c:398` |
| **512** | (a) pitch/gyro/v/x không hữu hạn | 1 tick | `robot_fsm.c:275` |
| **512** | (b) gyro hoặc \|a\|² không hữu hạn | — | `balance_motion.h:78` |
| **512** | (c) giá trị REC1 không hữu hạn | — | `balance_recovery.h:55` |
| **512** | (d) `BalanceMotion8_Update` trả false | — | `balance_motion.h:201` → `robot_fsm.c:342` |
| **512** | (e) `motion_cat_step8_mode()` fail | — | `robot_fsm.c:380` |
| **512** | (f) position/cancel_origin không hữu hạn | — | `balance_motion.h:60` |
| **2048** | Capture lúc ARM không đạt im lặng | 3 000 ms (\|θ_ARM\| ≤ 58°) / 4 000 ms (> 58°); im = 200 ms `\|pitch\|≤8°, \|gyro\|<25, \|v\|<0,15` | `robot_fsm.c:308`, `balance_startup.h:26-34` |
| **4096** | (a) REC1 kéo dài | 4 000 ms; thoát khi 300 ms im (`\|pitch\|≤6, \|gyro\|<25, \|Δv\|<0,15, \|x−x_ref\|≤budget`) | `balance_recovery.h:82-90` |
| **4096** | (b) phanh quỹ đạo không kết thúc | 4 000 ms; "xong" = 20 tick `\|v\|<0,06 & \|v_ref\|<0,01 & \|pitch\|≤6 & \|gyro\|<25`. **RT29 miễn nếu `braking && hold_anchor`** (phanh do REC1/bị đẩy) | `balance_motion.h:73-74` → `robot_fsm.c:348` |
| **8192** | `\|gyro\|` > 900 °/s sau khi đã có lệnh | 900 °/s | `balance_motion.h:81` → `robot_fsm.c:306` |

Hai mã **không** cắt motor, chỉ chặn ARM: **128** (gyro calibration, `robot_fsm.c:273`) và
**256** (cấu hình BMI160 sai, `robot_fsm.c:184`).

## 2. Dừng motor không ghi fault

| Nơi | Khi nào |
|---|---|
| `robot_fsm.c:394` | mỗi tick khi `state ≠ BALANCING` (INIT, CALIBRATING, STANDBY, FALLEN, EMERGENCY) |
| `robot_fsm.c:384` | tick đầu sau ARM (`just_armed`) — phanh 5 ms theo thiết kế |
| `robot_fsm.c:152` | `Robot_RecalibrateIMU()` — dừng trước khi đo gyro |
| `robot_fsm.c:79` | `Robot_Init()` lúc cấp nguồn |

## 3. Không phải cắt motor

- **Lease 350 ms** MOVE8 hết hạn → chỉ `v_ref` về 0, xe vẫn cân bằng.
- **REC1** (nghiêng 8°/12°, gyro 150 °/s, trượt tốc độ) → giới hạn mềm 50° / 1,2 m/s, motor vẫn chạy.
- **Governor** (cờ 16384), **interlock chưa đủ 200 ms**, **phanh quỹ đạo** → chỉ giảm PWM.

## 4. Ghi chú khi đọc log

- `fault` trong khung 122 B là tổng OR từ lần ARM gần nhất, nên một con số có thể mang nhiều bit.
- **4096 có hai nguồn**: REC1 hết 4 s và phanh quỹ đạo hết 4 s. Log RT28 `(12)(3)` tự ngắt 3 lần
  đều do nguồn thứ hai tại thời điểm xe đang dựng thẳng — đã sửa ở RT29
  (`Docs/RT29_Tu_Ngat_4096.md`).
- Legend mã lỗi trong header log vẫn in **1024** và **state 7 pulse**; hai mục đó **không còn** trong
  bản RT29 (không còn hằng số 1024, không còn chỗ nào đặt `state = 7`).
- Firmware **không có watchdog độc lập** và không có đường "vòng lặp chết → cắt motor". Nếu MCU rơi
  vào `Error_Handler` (`main.c:231`, `__disable_irq(); while(1)`), TIM4 ngừng nhưng TIM1 giữ duty cuối.
  Chưa từng xảy ra trong các log đã phân tích, nhưng nếu cần an toàn tuyệt đối thì phải thêm IWDG
  và tắt đầu ra TIM1 trong `Error_Handler` — việc này phải kiểm lại trên xe.
