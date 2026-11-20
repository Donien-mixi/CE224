# Bàn giao đồ án TWIP — 19/11/2026 (bản RT40)

# (dòng dưới là bản hiện hành RT41)
> **Bản hiện hành: STM32 `BAL-20261120-R12-8DIR-RT44-TURN` + ESP32 `TWIP-WEB-20261120-BAL-R12-8DIR-RT44-TURN-NET44`.**
> **RT44 (20/11):** log RT42+RT41 ở MAX 90, cùng lệnh 1,15 rad/s: yaw thực trải **0,90–1,38 rad/s**
> (lệch 53 %), vượt tới 20 %, dao động tới 0,38 rad/s; hai hướng xấu nhất cần **bánh trái** nhanh nhất
> (bánh trái cần ~11 % duty nhiều hơn). Sửa: hệ số dư duty nhân vào **cả tham chiếu yaw**, ngưỡng
> 96→**92 %**, hồi ≤86 %. Chi tiết: [`Firmware/RT44_TURN_20261120/README.md`](Firmware/RT44_TURN_20261120/README.md).
>
> **RT43: STM32 `BAL-20261120-R12-8DIR-RT43-MAX` + ESP32 `TWIP-WEB-20261120-BAL-R12-8DIR-RT43-MAX-NET43`.**
> **RT43 (20/11, từ log RT42 tại nhà — 148,6 s):** đường lệnh **đã đạt** (`age` p50 13 / p95 **27** /
> p99 40 / max **54 ms**; 0 lần khe >150 ms khi giữ lệnh; `# RADIO ch=6 rssi=-24 phy=11n ws_joins=1
> ws_leaves=0` = điện thoại không rớt lần nào) và REC1 chỉ còn **5,6 %**. Nhưng **2 lần ngã, cả hai ở
> MAX 90**: duty p95 0,90 (tới 33 % mẫu ≥0,96), vượt tham chiếu 0,95–0,96, bánh ghim 2499 rồi lệch
> trái/phải (0,91 vs 0,71 m/s) → thân quật ±55°. Pack ~11,6 V không đủ cho 0,90 m/s. RT43 thêm
> **trần MAX theo duty ở đường thẳng** (bánh ≥96 % → hạ tham chiếu, sàn 0,86 = 0,774 m/s; pack tươi
> ≤90 % duty thì không can thiệp nên vẫn đạt 0,90). Chi tiết:
> [`Firmware/RT43_MAX_20261120/README.md`](Firmware/RT43_MAX_20261120/README.md).
>
> **RT42: STM32 `BAL-20261120-R12-8DIR-RT42-RF` + ESP32 `TWIP-WEB-20261120-BAL-R12-8DIR-RT42-RF-NET42`.**
> **RT42 (20/11, chống nhiễu 2.4 GHz):** log RT41 (50,4 s) cho `age` p50 13 / p95 29 / p99 45 ms, `drops=0`,
> không lần nào >150 ms khi đang có lệnh → đường truyền đã ổn, thiếu là khả năng chịu nhiễu. RT42: (1) mỗi log
> mang dòng `# RADIO ch= rssi= phy= ws_joins= ws_leaves= ap_joins= ap_leaves= ap_reason=`; (2) `/network-status`
> có `rssi`/`phy`/`channel`; (3) `POST /network-channel` đổi kênh AP khi xe **đang đỗ**; (4) lease lệnh
> **350→500 ms** (`BALANCE_MOTION_STALE_MS`) để một cú nghen 300–400 ms không phanh oan; (5) HTTP poll
> **100→200 ms**. Không đổi gain/observer/phân bổ R12, REC1, interlock, khung 122 B. Chi tiết:
> [`Firmware/RT42_RF_20261120/README.md`](Firmware/RT42_RF_20261120/README.md).
>
> **RT41: STM32 `BAL-20261120-R12-8DIR-RT41-LINK` + ESP32 `TWIP-WEB-20261120-BAL-R12-8DIR-RT41-LINK-NET41`.**
> **RT41 (20/11, chỉ đường truyền):** UART STM32↔ESP32 **115200 → 921600** (khung telemetry 122 B giữ
> TX DMA 10,6 ms/mỗi 20 ms = **53 %** trước, còn **1,3 ms = 6,7 %**; lệnh 35 B 3,04 → **0,38 ms**) và
> **chọn kênh AP ít nhiễu** trong 1/6/11 (quét một lần, ≤ ~0,8 s, log `[NET41] … -> AP chN`,
> `/network-status` báo `channel`). **Không đổi một dòng nào của luật điều khiển** — `git diff` chỉ có
> `usart.c`, `.ioc`, tệp ESP32 và build ID; text/data/bss STM32 **giống hệt RT40**. Năm đề xuất còn lại
> (WS core 0, bỏ HTTP poll, payload nhỏ, lệnh nhị phân) **không làm** vì lý do đo được — xem
> [`Docs/RT41_Duong_Truyen.md`](Docs/RT41_Duong_Truyen.md) mục 4. Đo đường lệnh trên 6 log RT39
> (25 481 mẫu): `age` p50 13 ms / p95 28 ms, khoảng cách gói lệnh p95 35 ms, chỉ 0,1–0,2 % vượt 60 ms.
> **Phải nạp cả hai board** (UART đổi hai đầu). Bản nạp: [`Firmware/RT41_LINK_20261120/`](Firmware/RT41_LINK_20261120/README.md).

> **RT40: STM32 `BAL-20261119-R12-8DIR-RT40-HOLD` + ESP32 `TWIP-WEB-20261119-BAL-R12-8DIR-RT40-HOLD-NET40`.**
> Ba mức **50 / 80 / 90 cm/s**, bù lệch bánh trái **+110 PWM**, cua **0,72 m/s** (tự hạ thêm khi bánh
> sát 100 % duty), chống trôi khi đứng yên, đổi hướng liên tục, **không ngập ngừng khi phanh, không cắt
> motor khi bị giữ**. Bản nạp + hash: [`Firmware/RT40_HOLD_20261119/`](Firmware/RT40_HOLD_20261119/README.md).
>
> **RT40 (19/11, 8 log của RT39 — 1 100 s, cả 3 mốc và 8 hướng):** người vận hành báo đẩy xe thì
> **motor bị ngắt**, xe **ngập ngừng chưa thực thi ngay** khi nhận lệnh, và **rẽ liên tục 4 hướng ở 90
> thì ngã**. Đo được: **15/28 cước REC1** vào khi đang giữ lệnh chỉ vì độ ngả **12,1–15,1°** của cú
> phanh (gyro 1–61 °/s) → `v_ref` ghim 0 tới **2,2 s**, tổng **8,4 s “có lệnh mà không chạy”**; một lần
> cắt **fault 4096 đúng 4 s** khi xe vẫn đứng (bị giữ tay); **3 lần ngã** ở mốc 90 đều có một bánh ghim
> **duty 1,00** khi đang cua → nhả lệnh thì yaw brake làm trục vọt −0,72 → −0,95 m/s → thân quật −56°.
> Ba sửa: (1) ngưỡng nghiêng khi có lệnh **12° → 16°**; (2) hạn REC1 **4 s → 8 s**; (3) **trần cua theo
> duty còn dư** (hạ tham chiếu khi có bánh ≥96 % duty, sàn 0,72, hồi khi ≤90 %). Chi tiết:
> [`Docs/RT40_Het_Ngap_Ngung.md`](Docs/RT40_Het_Ngap_Ngung.md).

> **Bản hiện hành: STM32 `BAL-20261119-R12-8DIR-RT39-CORNER` + ESP32 `TWIP-WEB-20261119-BAL-R12-8DIR-RT39-CORNER-NET39`.**
> Ba mức tốc độ **50 / 80 / 90 cm/s**, bù lệch bánh trái **+110 PWM**, cua **0,719 m/s (72 cm/s)** ở cả
> hai chiều, chống trôi khi đứng yên, phanh nhả nhanh hơn, xuất phát mượt, đổi hướng liên tục,
> **mốc 80 vào cua không còn tự dừng**. Bản nạp + hash:
> [`Firmware/RT39_CORNER_20261119/`](Firmware/RT39_CORNER_20261119/README.md).
>
> **RT39 (19/11, log `twip_balance_R12 (39).txt` của RT38 — 4 198 mẫu / 84 s, preset 80, bốn hướng rẽ):**
> người vận hành báo xe **khựng khi cua và có lúc dừng rất lâu để cân bằng ở cả bốn hướng**. Log sạch
> (`crc 0`, `gaps 0`, 0 drops, `age` p50 13 ms, **0 fault** khi `state=3`) nhưng **30,2 % thời gian ở
> REC1** (12 cước, p50 1,94 s, max 2,76 s) và **18,8 % thời gian có lệnh mà |v| < 0,10 m/s**. Nguyên
> nhân: guard vượt tốc độ so bánh với **tham chiếu tức thời** (`|v|>0,78 & |sai|>0,04`, và
> `|v|>0,72 & |sai|>0,12`) — ở mốc 80 xe chạy bình thường 0,78–0,84 m/s, còn cua hạ tham chiếu xuống
> 0,749, nên **mỗi lần vào cua / nhả lệnh đều bị coi là vượt tốc độ**. Hai sửa: (1) **một** guard duy
> nhất so với **trần interlock** `max(envelope, |v_ref|+0,12)` và cần **100 ms** bằng chứng; (2) trần
> cua 0,90 → **0,87** để bánh ngoài còn ~10 % duty (duty từng ghim 1,00 ở 33 % số mẫu cua lùi-phải).
> Chi tiết: [`Docs/RT39_Cua_80_Khong_Khung.md`](Docs/RT39_Cua_80_Khong_Khung.md).
>
> **RT38 (18/11, phản hồi trên RT37):** ở 90 xe trôi/khựng; ở 50–80 đổi hướng đột ngột thì xe phải
> khựng giữ cân bằng. Log RT36 `(2)(5)` cho thấy ngay sau đảo chiều ±0,8–0,9 m/s có **72 % mẫu bị
> phanh** và **52 % vào REC1**, tốc độ không về 0,5 trong 1,2 s. Ba sửa: (1) giữ lệnh cũ tối đa
> **120 ms** khi nút gửi neutral (đổi hướng = đảo chiều liên tục, không phanh); (2) mốc vượt 20 cm thì
> **neo mốc và giữ tốc độ** khi xe đang chạy thay vì cắt về 0; (3) khi đang giữ lệnh, gyro ≥150 °/s phải
> duy trì **60 ms** mới vào REC1. Chi tiết: [`Docs/RT38_Chuyen_Huong_Lien_Tuc.md`](Docs/RT38_Chuyen_Huong_Lien_Tuc.md).
>
> **RT37 (18/11, log `(2)(5)` của RT36):** xuất phát mất 2,6–5,5 s mới đạt 90 % mốc, governor `limited`
> 25–44 % 2 s đầu, pitch 7–12° — nguyên nhân là ngưỡng nghiêng governor 9°/11° thấp hơn độ ngả bình
> thường khi tăng tốc. RT37 nâng thành **10°/13°**; thử giảm gia tốc mồi (0,70→0,45) hoặc nâng crawl
> (0,06→0,12) đều làm mô phỏng xấu đi (9–10/12), nên giữ nguyên ramp. Chi tiết:
> [`Docs/RT37_Xuat_Phat_Muot.md`](Docs/RT37_Xuat_Phat_Muot.md). Log đó cũng xác nhận **realtime đúng**:
> `dt` 4 996–5 009 µs, `exec` p95 648 µs, 0 drops, tuổi lệnh p95 27 ms.
>
> **RT36 (18/11) sửa 4 lỗi từ 4 log RT34:** (1) tiến chỉ 0,79–0,84 vì governor kẹp mốc khi áp lực do
> lag — giờ chỉ kẹp khi duty > 0,95; (2) cua trái chậm hơn cua phải vì cùng lý do; (3) cua phải-lùi
> ngập ngừng vì trần ngang tính theo `|v|` đo — giờ tính theo tốc độ lệnh; (4) đứng yên dao động 3–11 cm
> vì mốc cách 7 cm — giờ mốc tự chảy về vị trí sau 3 s đứng yên. Trần cua hạ 0,95 → 0,90 (≈0,75 m/s) và
> ramp phanh nhả 0,65 → 0,85 m/s². Chi tiết + số đo: [`Docs/RT36_Sua_4_Loi.md`](Docs/RT36_Sua_4_Loi.md).
>
> **Thay đổi so với bản bàn giao 17/11:** đã xoá hai worktree thử `D:\DA_HTN_speed086`
> (nhánh `test/speed-086` @ `7ec9f0e`) và `D:\DA_HTN_speed090` (`test/speed-090` @ `6bc90ac`) cùng hai
> nhánh local đó; bản chốt RT34 đã bao gồm mức 0,90 cao nhất đã thử.

## 0. Trạng thái một dòng

Xe đã chốt phần điều khiển: cân bằng, 3 mức tốc độ, 8 hướng, cua, chống đẩy, tự cắt an toàn.
**Không có thay đổi firmware nào đang dở.** Việc còn mở đều là phần cứng/cơ khí và một thao tác
format vùng log Flash (mục 6).

## 1. Đọc nhanh trong 5 phút

1. [`README.md`](README.md) — tổng quan, cách chạy, bảng đã kiểm.
2. Mục 2 dưới đây — kiến trúc điều khiển: FSM, pipeline 200 Hz, thứ tự ưu tiên.
3. Mục 3 — chức năng đang chạy kèm số đo, nhãn **đã kiểm trên xe** / chưa.
4. Mục 6 — việc còn mở; mục 7 — quy tắc bắt buộc khi sửa firmware.

Luật điều khiển chi tiết: [`Docs/Thuat_Toan_Dieu_Khien.md`](Docs/Thuat_Toan_Dieu_Khien.md).
Thông số cơ–điện–encoder: [`Docs/Thong_So_Xe_Hien_Tai.md`](Docs/Thong_So_Xe_Hien_Tai.md).

## 2. Kiến trúc điều khiển

### 2.1 Ba tầng thời gian

| Tầng | Chạy ở đâu | Làm gì | Hạn chót |
|---|---|---|---|
| `main()` super-loop | ngoài ngắt | parse UART, `$CALIB` (dừng TIM4), còi/LED, đẩy telemetry | mềm, không được block |
| `Robot_ControlLoop_200Hz` | ngắt TIM4, 5 ms | **toàn bộ điều khiển**: đọc cảm biến → quyết định → PWM | cứng 5 ms; vượt 4 ms thì tự cắt (fault 64) |
| ESP32-S3 | board phụ | web, WebSocket, log FFat | không nằm trong vòng hạn chót |

Nguồn: [`main.c:128`](Xe_Can_Bang/Core/Src/main.c#L128), [`robot_fsm.c:250`](Xe_Can_Bang/Core/Src/robot_fsm.c#L250).

### 2.2 FSM cấp cao — 8 trạng thái, bản chốt dùng 5

| Trạng thái | Vào khi | Ra khi | Việc làm |
|---|---|---|---|
| `INIT` = 0 | reset | BMI160 OK → `CALIBRATING`; lỗi I2C → `EMERGENCY` | cấu hình motor/encoder/I2C/UART |
| `CALIBRATING` | khởi động hoặc `$CALIB` | 500 mẫu gyro xong → `STANDBY` | chờ đứng yên, tính bias |
| `STANDBY` | hiệu chuẩn xong | ARM hợp lệ → `BALANCING` | chờ dựng xe, motor **dừng** |
| `BALANCING` | ARM + `ready` | `BalanceStop()` → `FALLEN` | chạy pipeline đầy đủ |
| `FALLEN` | mọi lỗi cắt máy | chỉ ARM lại / `$CALIB` | motor dừng, giữ lỗi trong `fault` |
| `EMERGENCY` | IMU chết, non-finite lúc init | `$CALIB` hoặc reset | còi báo động, không tự phục hồi |
| `RACING` = 4, `BENCH_TEST` = 7 | — | — | **không dùng** trong bản chốt |

`FALLEN` **không tự thoát**. ARM lại đòi: 200 ms đứng yên, `|pitch| ≤ 60°`, gyro ≤ 6 °/s, bánh
< 0,05 m/s, |a| trong 0,72–1,32 g, nhịp vòng 4–6 ms ([`robot_fsm.c:279`](Xe_Can_Bang/Core/Src/robot_fsm.c#L279)).

### 2.3 Pipeline 200 Hz — đúng thứ tự thực thi

```
TIM4 5 ms
 ├─ 1. DWT + loop_count                                  đo nhịp thật
 ├─ 2. BMI160_Read_All                                  lỗi → fault 32, EMERGENCY
 ├─ 3. Complementary filter (α = 0,005)                 pitch, gyro_rate
 ├─ 4. Encoder_Update                                   v_l, v_r, v_avg, x_actual
 ├─ 5. emergency_stop                                   → BalanceStop(1)
 ├─ 6. calib/config                                     → fault 128 / 256
 ├─ 7. isfinite                                         → fault 512
 ├─ 8. ARM gate 40 tick (200 ms)                        → ready
 ├─ 9. arm_request hợp lệ                               → BALANCING, reset tích phân
 ├─ 10. NẾU BALANCING — chuỗi veto, dừng ngay khi lỗi:
 │      a. heartbeat 10 s                              → 2
 │      b. va đập gyro > 900 °/s                        → 8192
 │      c. startup capture quá hạn                      → 2048
 │      d. REC1: impact / speed-reserve / update        → 4096
 │      e. governor: observe, rest, anchor, step        → ghìm, không cắt
 │      f. motion + turn update                         → 512 nếu NaN
 │      g. motion fault (kẹt, phanh quá 1,25 m/4 s)     → 8 / 4096
 │      h. |pitch| > 25° (50° khi REC1)                 → 4
 │      i. |x − x_ref| > 0,30 m (1,0 m khi REC1)        → 8
 │      j. speed interlock 200 ms / 1,45 m/s            → 16
 │      k. nhịp vòng ngoài 4–6 ms                       → 64
 ├─ 11. LÕI R12: input[7] → motion_cat_step8_mode → output[2]   (bất biến)
 ├─ 12. BalanceTrim_Left(output[0], 110 PWM)            bù cơ khí bánh trái
 ├─ 13. Motor_SetDuty(pwm_left, pwm_right)
 ├─ 14. nếu không BALANCING → Motor_Stop()
 ├─ 15. thời gian thực thi > 4 ms                       → 64
 └─ 16. mỗi 4 tick: 122 B + CRC → queue → ESP32         (50 Hz)
```

### 2.4 Thứ tự ưu tiên — chuỗi veto, không phải chuỗi chọn

| Mức | Lớp | Quyền tối đa | Tham số chốt |
|---|---|---|---|
| **P0** | Cắt an toàn | cắt cả 2 PWM ngay | 25°/50°, 0,30/1,0 m, 1,45 m/s, 4 ms, 200 ms debounce |
| **P1** | Startup capture + REC1 | nới trần góc/tốc độ/quãng **có hạn** | 50°, 1,20 m/s, +0,8 m, 4 s |
| **P2** | Governor | ép gia tốc về 0,06 m/s², hạ trần | duty > 0,90, lean 10°/13° |
| **P3** | Quỹ đạo 8 hướng | đặt `v_ref` | 0,50 / 0,80 / 0,90 m/s |
| **P4** | Lái / cua | đặt `w_ref`, `psi_ref` | 1,3–2,4 rad/s, trần bánh 0,80/0,98/1,05 |
| **P5** | **Lõi R12** | **nguồn PWM duy nhất** | `CAT_K`, `CAT_PARAM`, observer 14 trạng thái |
| **P6** | Trim cơ khí | cộng offset hằng, kẹp ±2499 | +110 PWM bánh trái |
| **P7** | Telemetry | chỉ đọc | 122 B / 50 Hz |

Đọc từ trên xuống: P0 thắng tất cả; P2 chỉ ghìm, không nâng; P5 luôn chạy kể cả khi `v_ref = 0`;
P6 là **khoản cộng duy nhất** trong hệ và là hằng số cơ khí đã đo, không phải gain.

## 3. Chức năng đang chạy

| Nhóm | Hành vi + tham số | Đã kiểm trên xe? |
|---|---|---|
| Cân bằng R12 | 14 trạng thái quan sát, `CAT_K = 9.1508/6.1289/14.3279/1.1605`, `CAT_PARAM[4] = 9,5653 N`, bộ lọc bù alpha 0,005 — **đóng băng từ RT8** | Có (nhiều log) |
| Tốc độ | 50 / 80 / 90 cm/s; gia tốc 0,60–0,70; giảm tốc 0,90; đảo chiều 1,0 m/s² | Có — log `(1)(5)`: 173 s, pack 11,7 V, 0 ngã, 0 tự ngắt. Khuyến nghị MAX 90 chỉ khi pin ≥ 12,0 V để còn dư địa |
| ARM / dựng xe | 200 ms đứng yên, 60°, gyro 6 °/s, bánh 0,05 m/s | Có |
| Startup capture | ngân sách 0,30 m, trần 25°, tốc độ 1,00/1,10/1,20 m/s, timeout 3–4 s | Có |
| Chống đẩy / nhiễu | REC1: gyro ≥ 150 °/s, hoặc (nghiêng ≥ 8°/12° hay tốc ≥ 0,35 m/s) kéo dài 60 ms khi đang có lệnh; nới 50°/1,20 m/s/+0,8 m/4 s; thoát sau 300 ms yên tĩnh; giữ mốc khi bị đẩy | Có — nhiều log, có cú va ~160 °/s vẫn phục hồi |
| Governor | áp lực khi duty > 0,90, lean dự đoán > **10°/13° (RT37)**, trễ bám > 0,12/0,30 m/s → kẹp gia tốc 0,06 m/s², chống deadlock 0,15 m/s. **RT36: chỉ neo mốc (10 cm) khi duty > 0,95**, phanh nhả 0,85 m/s² | Có (log 18/11 trước sửa) |
| Giám sát an toàn | interlock: trần `max(envelope; \|v_ref\|+0,12)` với envelope 1,12 khi MAX / 1,00–1,20 khi capture / 1,20 khi REC1, phải vượt liên tục 200 ms; 1,45 m/s cắt tức thì; ở MAX còn cắt nếu `\|v_bánh\| > 1,20`; cắt góc/quãng/I2C/vòng lặp; không tự ARM lại | Có |
| Bẻ hướng | quay tại chỗ 1,3/2,4 rad/s; chéo 0,90/1,15 rad/s; yaw accel 4,5, decel 8,0; trần ngang 0,9 m/s² **tính theo `max(\|v_ref\|,\|v\|−0,10)`**; taper lean 12°/4°; trôi tâm 45 cm/sàn 55 %; đóng băng heading ≤ 0,6 s | Có — log 18/11 trước sửa |
| Trần cua | đang cua: `v_ref ≤ 0,87 − \|w_ref\|·0,131` → **0,719** ở 1,15 rad/s; chỉ hạ, không nâng (RT39) | **Chưa kiểm trên xe** — đo trên log RT38: bánh ngoài 0,90 = 94–96 % duty, ghim 1,00 ở 33 % mẫu cua lùi-phải |
| Vượt tốc độ (RT39) | REC1 chỉ khi `\|v\| > max(envelope, \|v_ref\|+0,12)` **và** cùng dấu tham chiếu, đủ **100 ms** | Có — log RT39: chỉ 2 mẫu/1 100 s vượt trần, cả hai trong một REC1 |
| Ngưỡng nghiêng (RT40) | khi **có lệnh 16°**, khi **không lệnh 8°**, gyro 150 °/s, sai bám 0,35 m/s; REC1 cho 50°, cắt góc 25° | **Chưa kiểm trên xe** — 15/28 cước REC1 của RT39 là độ ngả phanh 12,1–15,1° |
| Hạn REC1 (RT40) | 8 s (trước 4 s) — chặn bởi 50°/1,0 m/interlock | **Chưa kiểm trên xe** — RT39 bị cắt fault 4096 khi bị giữ tay 4 s |
| Trần cua theo duty (RT40) | đang cua mà có bánh ≥96 % duty → hạ tham chiếu (sàn 0,72), hồi khi ≤90 %; chỉ khi cua | **Chưa kiểm trên xe** — 3 lần ngã của RT39 đều có bánh ghim 100 % duty |
| Giữ khi đứng yên | sau 3 s đứng yên thật, mốc chảy về vị trí đo 2 cm/s trong phạm vi 15 cm (RT36) | **Chưa kiểm trên xe** |
| Xuất phát / tăng tốc | ngưỡng nghiêng governor 10°/13° (RT37), không cắt gia tốc ở độ ngả 7–12° bình thường | **Chưa kiểm trên xe** |
| Đổi hướng liên tục | giữ lệnh cũ tối đa 120 ms khi nút gửi neutral; mốc vượt 20 cm thì neo mốc + giữ tốc độ khi xe đang chạy (RT38) | **Chưa kiểm trên xe** |
| REC1 khi đang giữ lệnh | gyro ≥150 °/s phải duy trì 60 ms mới vào REC1; không có lệnh vẫn vào ngay (RT38) | **Chưa kiểm trên xe** |
| Bù bánh trái | +110 PWM (đo +98 tiến / +160 lùi / +166 quay), dead-band 0,5, kẹp ±2499, đặt **sau** phân bổ R12 | Có — log RT30/RT31 |
| Log / web | 122 B, 40 cột, 50 Hz; `/ui/r12`; SoftAP; TX 11 dBm; chỉ ghi Flash khi xe dừng | Có — `reset=power_on`, hết brownout |
| Mô phỏng | dùng chính header firmware; đường bánh fit từ log `(1)(5)` (lệch 1–2 %) | Không áp dụng |

## 4. Repo, build, nạp

### 4.1 Trạng thái git

| Mục | Giá trị |
|---|---|
| `origin/main` | bằng `main` — commit RT36 mới nhất (xem `git log -1`) |
| `main` local | bằng `origin/main` (không đi trước, không đi sau) |
| Cây làm việc | sạch (0 thay đổi) |
| Nhánh | `main`, `pcb` (PCB đã push ở `origin/pcb`) |
| Nhánh đã xoá 18/11 | `test/speed-086` @ `7ec9f0e`, `test/speed-090` @ `6bc90ac` — còn trong reflog ~90 ngày |
| Tệp được theo dõi | 466 |
| Thư mục build local | `.build/` 156 MB, `Xe_Can_Bang/Debug/` 29 MB, `.metadata/` 7 MB — **đã gitignore**, xoá được nếu cần chỗ |

### 4.2 Bản nạp và hash

| Bản | Thư mục | Khi nào dùng |
|---|---|---|
| **RT43-MAX / NET43** (hiện hành) | `Firmware/RT43_MAX_20261120/` | Chạy bình thường; hết ngã ở MAX 90 |
| RT42-RF / NET42 (trước) | `Firmware/RT42_RF_20261120/` | Chống nhiễu 2.4 GHz, log `# RADIO` |
| RT41-LINK / NET41 (trước) | `Firmware/RT41_LINK_20261120/` | UART 921600, kênh chọn lúc khởi động |
| RT40-HOLD / NET40 (trước) | `Firmware/RT40_HOLD_20261119/` | Luật điều khiển y hệt RT41 nhưng UART 115200, kênh AP 6 |
| RT39-CORNER / NET39 (trước) | `Firmware/RT39_CORNER_20261119/` | Bản đã có 8 log (1 100 s) — nguồn của 3 lỗi RT40 sửa |
| RT38-CONT / NET38 (trước) | `Firmware/RT38_CONT_20261118/` | Bản đã có log `(39)` 84 s (chính là ca khựng ở 80 mà RT39 sửa) |
| RT37-SMOOTH / NET37 (trước) | `Firmware/RT37_SMOOTH_20261118/` | Bản thử ngay trước RT38 |
| RT36-SYM / NET36 (trước) | `Firmware/RT36_SYM_20261118/` | Bản đã có log 154 s (4 kịch bản) |
| RT34-CORNER / NET35 (trước) | `Firmware/RT34_CORNER_20261117/` | Bản đã có log xe 173 s |
| RT29-NOCUT / NET30 (dự phòng) | `Firmware/RT29_NOCUT_20261115/` | Quay về bản cũ nhất: ba mức 50/72/80, chưa có bù bánh trái và trần cua |

Kiểm ngày 20/11 (RT41): **7/7** tệp binary khớp `SHA256.json`; **15/15** tệp nguồn khớp
`SOURCE_SHA256.json` → bản `.bin/.hex` được build từ **đúng source hiện tại**. Build lại trong
workspace CubeIDE `D:\CubeIDE_ws` ra `.hex` **trùng hash** bản đóng gói (`4b1efacf…`).
`test_build_id_sync` PASS: 9 literal trên 55 tệp. Build IDE trong workspace sạch
`D:\CubeIDE_ws_verify` (workspace chính đang mở trong CubeIDE) ra hex `d65d152c…` trùng bản đóng gói.

### 4.3 Build

```powershell
# STM32 (CubeIDE headless; workspace đã link tới D:/DA_HTN/Xe_Can_Bang)
& "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\headless-build.bat" -data D:\CubeIDE_ws `
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -import D:\DA_HTN\Xe_Can_Bang -build Xe_Can_Bang/Debug

# ESP32-S3 (arduino-cli 1.5.1 đi kèm Arduino IDE, không có trên PATH; sketchbook D:\Arduino có WebSockets 2.7.2)
& "C:\Users\DONG NHIEN\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" `
  compile --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi" `
  --libraries D:\Arduino\libraries --output-dir .build\esp32_out ESP32_S3_Gateway
```

> **Bẫy đóng gói:** build headless của CubeIDE cập nhật `.elf` nhưng **không chạy lại** bước
> `objcopy` — `.bin/.hex` trong `Xe_Can_Bang/Debug/` có thể vẫn là bản cũ. Sau mỗi build phải:
> `arm-none-eabi-objcopy -O binary Xe_Can_Bang.elf Xe_Can_Bang.bin` và `-O ihex … .hex`
> (objcopy nằm trong `C:\ST\STM32CubeIDE_2.1.1\…\gnu-tools-for-stm32…\tools\bin`), rồi mới tính hash.
> Đã xảy ra đúng lúc đóng gói RT36: .bin/.hex còn là bản RT34 trong khi .elf đã mới.

### 4.4 Kiểm chứng

```powershell
Get-ChildItem tools/test_*.cjs | ForEach-Object { node $_.FullName }   # 41/41
node tools/test_build_id_sync.cjs

node tools/run_governor_sim.cjs --stress                               # 12 ca
node tools/run_governor_sim.cjs --stress --edge                        # 12 ca
node tools/run_governor_sim.cjs --stress --launch --launch50           # 12 ca
```

- Host test: **41/41 pass** (Node.js 24, GCC msys2), gồm 8 test RT36–RT40 (`test_rt36_anchor`,
  `test_rt36_lateral`, `test_rt36_hold`, `test_rt36_brake`, `test_rt37_smooth`, `test_rt38_continuous`,
  `test_rt38_recovery`, `test_rt39_corner`, `test_rt40_hold`) — mỗi test có old-constant probe phải fail.
- Mô phỏng, chạy lại sau RT40 ngày 19/11: **12/12** ca đủ 20/20 s ở MAX 80, **12/12** `--edge`,
  **12/12** `--launch50` (pitch từng ca **trùng khớp RT39**); ở MAX 90 vẫn **11/12** — ca 7 ngã ở 8,87 s
  (RT39: 8,95 s; pin 11,2 V). Kết quả ở `Simulation/results/runs/…`, đếm ca từ `summary.csv`.

### 4.5 Nạp

Nháy đúp `nap_rt44.bat` trong thư mục bản nạp → nhập COM của ESP32 (**bắt buộc** cho RT41/RT42). Script nạp STM32 bằng
`STM32_Programmer_CLI.exe`, ESP32 bằng `esptool` v5 (`write-flash`) tại `0x0 / 0x8000 / 0xe000 / 0x10000`.
Kiểm sau khi nạp: banner `RT44-TURN-NET44`, ba nút **50 / 80 / MAX 90**, nút ĐO LẠI GYRO bấm được,
Serial ESP32 có `[UART] … @ 921600 bps` và `[NET41] … -> AP chN`, log có `crc_errors=0`.

Chín bản (RT43 / RT42 / RT41 / RT40 / RT39 / RT38 / RT37 / RT34 / RT29) đều có sẵn đủ file hai board; bảng chọn bản, cách nạp từng bản, kiểm
banner và bảng quay lui ở [`Firmware/README.md`](Firmware/README.md) — hoặc chạy menu
`Firmware/nap_chon_ban.bat`. Nạp bằng IDE (CubeIDE Build+Run / Arduino IDE Upload):
[`Docs/Nap_Bang_IDE.md`](Docs/Nap_Bang_IDE.md) — workspace RT43 là `D:\CubeIDE_ws`, RT42 là
`D:\CubeIDE_ws_rt42`, RT41 là
`D:\CubeIDE_ws_rt41`, RT40 là `D:\CubeIDE_ws_rt40`, RT39 là `D:\CubeIDE_ws_rt39`, RT38 là `D:\CubeIDE_ws_rt38`.

## 5. Quy tắc vận hành

1. **Đo pin bằng tay trước mỗi phiên.** MAX 90 chỉ khi **≥ 12,0 V**; 11,7–12,0 V dùng 50/80;
   dưới 11,7 V chỉ 50/80. (Không có đo áp trên xe — `batt_voltage` luôn 0, không được bịa số.)
2. **Giảm tốc rồi mới đảo chiều / buông ga** ở tốc độ cao. Hai cú ngã từng ghi nhận đều do đảo chiều
   đột ngột ở 0,85–0,90 m/s (PWM nhảy 100 %, thân quật 18–28°).
3. Sau mỗi lượt: **STOP → chờ ~2 s → tải log** (gateway chỉ ghi Flash khi đã dừng).
4. **Đừng bấm “SỬA VÙNG LOG FLASH” khi xe đang cân bằng** — format 9,9 MB làm ESP32 khởi động lại
   một lần (hành vi đã biết, không phải lỗi).
5. Mất kết nối Wi-Fi: STM32 vẫn tự cân bằng, nhưng sau **10 s** không nhận lệnh sẽ dừng motor.
   Đừng cố giữ ga trong lúc mất kết nối.
6. Một lượt log chỉ dùng được khi mở đầu bằng `BINFO` có đúng build ID; log thiếu build ID hoặc
   lẫn hai bản thì không so sánh.

## 6. Việc còn mở (theo thứ tự)

1. **RT40 chưa có log xe.** Cần một buổi đo: (a) đứng yên 30 s, đẩy nhẹ 1 lần, rồi **giữ 6–8 s**;
   (b) mỗi mốc 50/80 tiến 3 s → nhả → lùi 3 s → nhả; (c) đổi hướng liên tục ở 50/80; (d) rẽ 4 hướng ở
   80 và 90 (≥ 2 s mỗi hướng). Đạt khi: REC1 do nghiêng-khi-có-lệnh **0 cước** (RT39: 15), không cắt
   motor khi bị giữ (RT39: 1 lần fault 4096), **0 lần ngã** (RT39: 3), duty p95 khi cua **< 0,95**
   (RT39: 1,00), `v_ref` không ghim 0 quá 0,3 s khi đang giữ lệnh (RT39: tới 2,2 s).
2. **`archive_error=1`** trong mọi log gần đây → vùng log Flash chưa format lại. Cần bấm
   “SỬA VÙNG LOG FLASH” **một lần khi motor đã dừng** (xe đứng yên, chỉ ESP32 chạy), rồi kiểm
   `archive_saved > 0` trong log kế tiếp.
3. **Nguồn 5 V/3V3 tại ESP32**: thêm 1000 µF low-ESR + 100/10 µF, cấp từ nhánh riêng. Brownout đã
   hết nhờ hạ TX 11 dBm nhưng đây vẫn là gốc rễ phần cứng ([RT33](Docs/RT33_On_Dinh_Ket_Noi.md)).
4. **Nghiêng khi cua trái** còn lớn hơn cua phải (~5° so với ~1,4°) dù tốc độ quay đã cân — phần
   cơ khí (motor/hộp giảm tốc trái), không sửa được bằng gain. Log RT38 `(39)` định lượng thêm: bánh
   **trái cần ~11 % duty/1 m/s** nhiều hơn bánh phải ở **mọi** hướng (1,135 so với 1,012 ở lùi-phải);
   trim 110 PWM chỉ bù ~4,4 %, và tăng trim sẽ đẩy bánh trái vào bão hoà — nên RT39 **giảm tải** thay
   vì tăng trim. Cần kiểm bạc đạn/belt/motor trái.
5. **Tốc độ > 0,90 m/s**: cần pack 4S (3S chỉ ~0,96 m/s không tải; 0,90 ở 11,2 V cần duty 0,955,
   chiều lùi cần 12,23 V — ngoài dải pin).
6. **PCB**: chưa có bo thật để kiểm nhiễu/nhiệt/tải dòng.
7. **Không có watchdog độc lập**: nếu MCU treo, TIM1 giữ duty cuối. Cân nhắc IWDG ở phiên sau
   (phải đảm bảo không cắt oan trong lúc hiệu chuẩn 5 s).

## 7. Quy tắc cho phiên sau (bắt buộc)

- **Không sửa bất biến R12**: `CAT_K`, `CAT_PARAM`, observer 14 trạng thái, bộ phân bổ lực,
  `CAT_VNL`, alpha bù 0,005, khung telemetry 122 B. Mọi tính năng mới phải là **lớp giám sát**,
  chỉ có quyền cắt/giữ/giảm.
- **Đổi hành vi thì bump build ID cả hai board** (`balance_profile.h` và `firmware_build.h`) và
  **mọi literal** trong docs/test/script; `test_build_id_sync` phải PASS.
- Chạy **41 host test**, và với test mới phải có **old-constant probe**: test phải FAIL với hằng số
  cũ (bằng chứng nó thật sự kiểm tra chính sách mới).
- Sửa gì ảnh hưởng quỹ đạo/tốc độ thì chạy lại bộ mô phỏng tương ứng; **không** gọi kết quả mô phỏng
  là “đã kiểm trên xe”.
- **Không tự nạp board, không tự push `main`** nếu người dùng chưa yêu cầu. Nhánh thử đặt tên
  `test/…` và không push.
- Mọi báo cáo: nêu **build ID, n mẫu, điều kiện, hash**; tách rõ **đã kiểm trên xe / mới mô phỏng /
  chưa kiểm**; ghi cả việc **không** làm (không nạp, không push).

## 8. Tài liệu

| Tài liệu | Nội dung |
|---|---|
| [`Docs/Tong_Quan_Do_An.md`](Docs/Tong_Quan_Do_An.md) | Tổng quan đồ án, kiến trúc hệ |
| [`Docs/Tinh_Nang_Xe_Hien_Hanh.md`](Docs/Tinh_Nang_Xe_Hien_Hanh.md) | Chức năng đang chạy kèm số đo |
| [`Docs/Thuat_Toan_Dieu_Khien.md`](Docs/Thuat_Toan_Dieu_Khien.md) | Luật điều khiển và các lớp giám sát |
| [`Docs/Thong_So_Xe_Hien_Tai.md`](Docs/Thong_So_Xe_Hien_Tai.md) | Cơ khí, điện, encoder, đường đặc tính bánh |
| [`Docs/Bang_Ma_Loi_Tu_Dong_Dung.md`](Docs/Bang_Ma_Loi_Tu_Dong_Dung.md) | Mọi trường hợp firmware tự cắt motor |
| [`Docs/Phan_Cung_Va_Ket_Noi.md`](Docs/Phan_Cung_Va_Ket_Noi.md) | Phần cứng và đấu dây |
| [`Huong_Dan_Di_Day_Toan_Bo_He_Thong.md`](Huong_Dan_Di_Day_Toan_Bo_He_Thong.md) | Hướng dẫn đi dây toàn bộ hệ |
| [`Docs/Roadmap_Do_An.md`](Docs/Roadmap_Do_An.md), [`Docs/Phat_Trien_Tiep.md`](Docs/Phat_Trien_Tiep.md) | Phương pháp và hướng tiếp |
| [`Docs/RT36_Sua_4_Loi.md`](Docs/RT36_Sua_4_Loi.md) | 4 lỗi từ log 18/11: số đo trước/sau, cách sửa, giới hạn |
| [`Docs/RT37_Xuat_Phat_Muot.md`](Docs/RT37_Xuat_Phat_Muot.md) | Ngập ngừng khi xuất phát/tăng tốc, kiểm tra realtime, 3 phương án đã thử |
| [`Docs/RT38_Chuyen_Huong_Lien_Tuc.md`](Docs/RT38_Chuyen_Huong_Lien_Tuc.md) | Khựng khi đổi hướng và ở 90 cm/s; số đo, 3 sửa, giới hạn |
| [`Docs/RT39_Cua_80_Khong_Khung.md`](Docs/RT39_Cua_80_Khong_Khung.md) | 80 cm/s khựng/dừng lâu khi cua ở cả 4 hướng; 12 cước REC1, bất đối xứng bánh trái, 2 sửa |
| [`Docs/RT40_Het_Ngap_Ngung.md`](Docs/RT40_Het_Ngap_Ngung.md) | 8 log RT39 (1 100 s): 8,4 s ngập ngừng do độ ngả phanh, cắt motor khi bị giữ, 3 lần ngã khi rẽ ở 90 vì bánh ghim duty |
| [`Docs/RT41_Duong_Truyen.md`](Docs/RT41_Duong_Truyen.md) | **Mới:** đo đường lệnh (age p95 28 ms), UART 921600, chọn kênh Wi-Fi, và lý do không làm 5/7 đề xuất |
| Báo cáo vòng | [RT22](Docs/RT22_Doi_Xung_Va_Be_Huong.md) · [RT23](Docs/RT23_Nut_That_Va_Loi_Toc_Do.md) · [RT24](Docs/RT24_Taper_Nghieng_Quay_Trai.md) · [RT27](Docs/RT27_Tro_Ve_Nen_RT21_Chong_Chiu.md) · [RT28](Docs/RT28_Giu_Moc_Vi_Tri_Khi_Bi_Day.md) · [RT29](Docs/RT29_Tu_Ngat_4096.md) · [RT32](Docs/RT32_Bo_Toc_Do_50_80_90.md) · [RT33](Docs/RT33_On_Dinh_Ket_Noi.md) · [RT34](Docs/RT34_Cua_Muot.md) · [RT35](Docs/RT35_Mo_Phong_Khop_Log.md) · [RT36](Docs/RT36_Sua_4_Loi.md) · [RT37](Docs/RT37_Xuat_Phat_Muot.md) · [RT38](Docs/RT38_Chuyen_Huong_Lien_Tuc.md) · [RT39](Docs/RT39_Cua_80_Khong_Khung.md) · [RT40](Docs/RT40_Het_Ngap_Ngung.md) · [RT41](Docs/RT41_Duong_Truyen.md) |

## 9. Lịch sử

Lịch sử đầy đủ (RT7 → RT21, các bản nạp cũ, log thô, nhánh thử đã xoá) nằm trong **git history**
của repo, không giữ trong cây làm việc. Commit tip của hai nhánh thử đã xoá: `7ec9f0e`, `6bc90ac`.
