# RT39 — Mốc 80 cm/s vào cua không còn khựng và không còn dừng lâu

Người vận hành báo trên **RT38**: ở mốc **80 cm/s**, xe bị **khựng khi cua/đánh lái**, có lúc **dừng
rất lâu để cân bằng ở cả bốn hướng rẽ**. Yêu cầu kèm theo: *những chức năng đã chạy tốt thì giữ
nguyên*, và kiểm lại toàn bộ bốn hướng đi của mốc 80.

Bằng chứng: log **`twip_balance_R12 (39).txt`** — RT38 `BAL-20261118-R12-8DIR-RT38-CONT`, gateway
NET38, 4 198 mẫu / 83,9 s, preset 80, cả bốn hướng rẽ.

## 1. Log `(39)` có sạch không

| Kiểm tra | Kết quả | Kết luận |
|---|---|---|
| `crc_errors` / `sequence_gaps` / `STM32_resets` / `drops` | 0 / 0 / 0 / 0 | đường truyền tốt |
| `dt_us` | 4 997–5 003 µs | vòng 200 Hz đúng |
| `execution_us` p95 | 655 µs | tải CPU thấp |
| `age` (t_ms − drive_rx_ms) p50 / p95 / max | 13 / 24 / 59 ms | lệnh không mất lease |
| `fault` khi `state==3` | **0 mẫu** | không có cắt motor, không fault 8/16/4096 |
| Tư thế trung tính khi đứng yên | **+1,82°** | mọi ngưỡng nghiêng phải hiểu là lệch so với mốc này |

Vậy không phải motor yếu, không phải encoder, không phải mất gói. Vấn đề nằm ở luật giám sát.

## 2. Số đo triệu chứng

| Số đo (preset 80, cả bốn hướng) | Giá trị |
|---|---|
| Thời gian ở REC1 (`control_phase=2`) | 1 267 mẫu = **25,3 s = 30,2 %** cả lượt chạy |
| Số cước REC1 | 12 (p50 **1,94 s**, p90 2,71 s, max 2,76 s) |
| Cước vào **trong lúc đang giữ lệnh** | 11/12 |
| Cước vào **ngay sau khi nhả lệnh** (phanh tham chiếu) | 2 |
| "Có lệnh mà \|v\| < 0,10 m/s" | 12,5 s = **18,8 %** thời gian có lệnh |
| Duty khi chạy thẳng 0,80 m/s | p50 **0,91** |
| Duty khi cua | p50 **0,94**, p95 **1,00**, 33 % số mẫu cua lùi-phải **ghim 1,00** |
| Bánh ngoài khi cua (v + \|w\|·0,131) | p50 0,86–0,87, p95 **0,95** |

Từng cước REC1 (thời điểm, hướng, tốc độ và tham chiếu ở nhịp ngay trước khi vào):

| t (s) | dir/turn | v | v_ref | \|v−v_ref\| | pitch | ngưỡng nào chạm |
|---:|---|---:|---:|---:|---:|---|
| 369,69 | +1/0 | 0,23 | 0,39 | −0,16 | 12,1° | nghiêng khi có lệnh (12°) |
| 378,75 | 0/0 (vừa nhả) | −0,77 | −0,65 | **−0,13** | 3,3° | nhánh vượt tốc độ cũ (\|v\|>0,72 & \|sai\|>0,12) |
| 385,43 | +1/−1 | 0,78 | 0,749 | **+0,04** | 5,6° | nhánh cũ (\|v\|>0,78 & \|sai\|>0,04) |
| 392,73 | +1/+1 | 0,78 | 0,754 | **+0,04** | 5,9° | nhánh cũ |
| 398,35 | −1/+1 | −0,77 | −0,65 | **−0,12** | 3,7° | nhánh vượt tốc độ cũ |
| 402,63 | 0/0 | 0,03 | −0,29 | +0,26 | 22,1° | va chạm thật (gyro 355 °/s) — **đúng** |
| 404,13 | +1/0 | −0,11 | +0,26 | **−0,37** | 6,2° | mất bám đuổi (hệ quả cước trên) |
| 415,73 | +1/−1 | 0,76 | 0,742 | +0,02 | 12,4° | nghiêng khi cua đang bão hoà duty |
| 429,07 | +1/−1 | 0,72 | 0,46 | **+0,26** | 7,6° | nhánh vượt tốc độ cũ (ramp sau REC1) |
| 431,65 | +1/−1 | −0,23 | +0,14 | **−0,37** | 3,4° | mất bám đuổi (xe lăn ngược) |
| 435,25 | +1/−1 | 0,12 | 0,19 | −0,08 | 12,8° | nghiêng khi có lệnh |
| 445,85 | +1/+1 | 0,77 | 0,73 | **+0,04** | 7,2° | nhánh cũ |

## 3. Nguyên nhân

### 3.1 Guard vượt tốc độ so với *tham chiếu tức thời* (6/12 cước)

`BalanceRecovery_SpeedReserve` vào REC1 khi `|v| > 0,72` **và** `|v − v_ref| > 0,12`, và
`BalanceRecovery_Reserve` khi `|v| > 0,78` **và** `|v − v_ref| > 0,04`. Ở mốc 80 xe chạy bình thường
**0,78–0,84 m/s** (duty 0,91), còn khi cua thì `BalanceMotion_corner_budget` hạ tham chiếu xuống
0,749 — nên **mọi lần vào cua** đều có sai lệch 0,04–0,06 m/s và **mọi lần nhả lệnh** đều có sai lệch
0,12 m/s. Guard đọc đó là "xe chạy nhanh hơn lệnh" và phanh 0,78 → 0 trong 1,9–2,8 s.

Đo kiểm chứng: trên toàn log, bánh chỉ **thật sự** vượt trần interlock `max(envelope, |v_ref|+0,12)`
ở **2 mẫu duy nhất trong 4 198 mẫu**, và cả hai đều nằm *trong* một cước REC1 đang chạy (do chính
REC1 đã hạ tham chiếu). Tức là vùng 0,78–0,87 m/s là vùng chạy bình thường, không phải vượt tốc độ.

### 3.2 Vòng lặp dương tính REC1 → hạ trần → lại REC1

Cước 429,07: sau REC1 trước, `BalanceMotion` giới hạn tốc độ còn 0,50 m/s trong 1,5 s (cờ 32768),
thân xe vẫn lăn 0,75 m/s → sai lệch 0,26 → guard cũ lại vào REC1 → lại gia hạn trần 0,50 → lại lệch.
Kết quả là **11,5 s** chỉ chạy 0,36 m/s trung bình với 4 cước REC1 liên tiếp. Đây là lý do "dừng rất
lâu" chứ không phải từng cước riêng lẻ.

### 3.3 Trần cua đặt bánh ngoài đúng bằng giới hạn động cơ

`BALANCE_TURN_VNL_BUDGET = 0,90` ⇒ tham chiếu cua `0,90 − 1,15·0,131 = 0,749` và **bánh ngoài đúng
0,90 m/s**. Với pack 11,2–11,7 V, 0,90 m/s cần **94–96 % duty**, không còn dư cho vòng cân bằng: duty
ghim 1,00 ở 33 % số mẫu cua lùi-phải, thân nghiêng 6–12° (có lúc 16°), và cước 415,73 tự vào REC1 vì
nghiêng. Đây là cơ chế thứ hai, độc lập với 3.1.

### 3.4 Bất đối xứng cơ khí còn lại (không sửa được bằng luật)

Duty trên một đơn vị tốc độ bánh, đo trên 6 tổ hợp hướng:

| Tổ hợp | \|duty/v\| bánh trái | \|duty/v\| bánh phải |
|---|---:|---:|
| dir=+1 turn=+1 | 1,120 | 1,070 |
| dir=+1 turn=−1 | 1,080 | 1,079 |
| dir=−1 turn=+1 | **1,135** | 1,012 |
| dir=−1 turn=−1 | 1,115 | 0,996 |

Bánh **trái** cần ~11 % duty nhiều hơn bánh phải cho cùng tốc độ, ở mọi hướng. Trim hiện tại
(`BALANCE_TRIM_LEFT_PWM 110` = +4,4 % duty) chỉ bù được một phần. RT39 **không** tăng trim (tăng sẽ
đẩy bánh trái vào bão hoà ở hướng lùi-phải, nơi nó đã 0,98 duty); chỉ giảm tải để nó không ghim 100 %.

## 4. Sửa gì (chỉ lớp giám sát, không đổi R12)

| # | Tệp | Thay đổi |
|---|---|---|
| 1 | [`balance_recovery.h`](../Xe_Can_Bang/Core/Inc/balance_recovery.h) | Gộp hai guard vượt tốc độ thành **một** `BalanceRecovery_OverSpeed`: chỉ vào REC1 khi `|v| > BalanceInterlock_Ceiling(envelope, v_ref)` (đúng trần mà interlock bảo vệ) **và** cùng dấu với tham chiếu, **và** đủ **100 ms** (`BALANCE_RECOVERY_OVERSPEED_TICKS 20`). Bỏ điều kiện nghiêng `|pitch|>5 or |gyro|>60` — nó không nhận ra xe chạy thẳng lăn nhanh trên thân phẳng |
| 2 | [`balance_recovery.h`](../Xe_Can_Bang/Core/Inc/balance_recovery.h) | Thêm trường `overspeed` (bộ đếm 100 ms) vào `BalanceRecovery`; xoá `BalanceRecovery_Reserve` (trùng luật với #1) |
| 3 | [`balance_governor.h`](../Xe_Can_Bang/Core/Inc/balance_governor.h) | Xoá `BalanceGovernor_ReserveNeeded` (chỉ phục vụ guard đã bỏ) |
| 4 | [`balance_turn.h`](../Xe_Can_Bang/Core/Inc/balance_turn.h) | `BALANCE_TURN_VNL_BUDGET` 0,90 → **0,87**: cua còn **0,719 m/s (72 cm/s)**, bánh ngoài 0,87 m/s ≈ 88–90 % duty, dư ~10 % cho vòng cân bằng |
| 5 | [`robot_fsm.c`](../Xe_Can_Bang/Core/Src/robot_fsm.c) | Gọi `BalanceRecovery_OverSpeed` **trước** `BalanceMotion8_Update` và trước interlock, truyền `BalanceStartup_SpeedLimit` (hoặc 1,12 khi max-envelope) |

Vì sao vẫn an toàn: bằng chứng 100 ms **ngắn hơn** 200 ms của interlock (`BALANCE_SPEED_TRIP_TICKS 40`),
và khi REC1 bật thì interlock lấy trần `1,20 m/s` ngay trong cùng nhịp — nên một cú đẩy thật vẫn vào
REC1 có giới hạn (góc 50°, tốc độ 1,20, +0,8 m) **trước khi** motor bị cắt. Ngưỡng cứng 1,45 m/s và
`BalanceRecovery_Impact` (gyro > 250 °/s + \|a\| bất thường) không đổi.

Không đổi gain/observer/phân bổ lực R12, neo mốc, phanh 0,85, trim 110 PWM, khung 122 B, lease 350 ms,
ngưỡng REC1 do nghiêng (12° khi có lệnh / 8° khi không), ngưỡng gyro 150 °/s, interlock 200 ms.

## 5. Kiểm chứng

- **40/40 host test pass**.
  - `test_rt39_corner` (mới): 438 mẫu 50 Hz **thật** cắt 1,5 s ngay trước 6 cước REC1 sai → **0 mẫu**
    vào REC1; guard vẫn bắt runaway 0,95 m/s trên tham chiếu 0,50 sau đúng 20 nhịp; xe lăn ngược
    chiều lệnh không bị coi là runaway; trần cua 0,719 m/s và bánh ngoài 0,87. Hai probe phải fail:
    `SLACK=0.12` (luật cũ) và `VNL_BUDGET=0.90`.
  - `test_rt12_replay72` (sửa): 38 ảnh chụp REC1 72 cm/s → 0 mẫu vào REC1; probe `SLACK=0.05` fail.
  - `test_rt34_corner`, `test_rt17_motion`, `test_balance_governor` cập nhật theo số mới.
- **Mô phỏng Scilab** (`tools/run_governor_sim.cjs`):

| Bộ | RT39 | RT38 (tham chiếu) |
|---|---|---|
| `--stress` @MAX 80 | **12/12**, mọi ca 20 s | 12/12 |
| `--stress` @MAX 90 | 11/12 — ca 7 ngã ở 8,95 s, pitch 62,5° | **giống hệt** (11/12, 8,95 s, 62,5°) |
| `--stress --edge` | **12/12** | 12/12 |
| `--stress --launch --launch50` | **12/12**, pitch từng ca **trùng khớp** RT38 | 12/12 |

- **Build**: STM32 `text 82 920 B` → bin 83 420 B, 0 lỗi 0 cảnh báo; ESP32 1 091 072 B.
- **Hash**: đóng gói ở ``Firmware/RT39_CORNER_20261119``.

## 6. Cần log gì để xác nhận

Một file `twip_balance_R12*.txt` mới, **đúng RT39** (`$BINFO` có `RT39-CORNER`), ở mốc **80**, chạy
đủ **bốn hướng** (tiến-trái, tiến-phải, lùi-trái, lùi-phải), mỗi hướng giữ lệnh ≥ 2 s rồi nhả.

| Chỉ số | RT38 `(39)` | Kỳ vọng RT39 |
|---|---:|---:|
| `control_phase=2` (% thời gian) | 30,2 % | **< 5 %** |
| Cước REC1 dài nhất | 2,76 s | **< 0,5 s** |
| "có lệnh mà \|v\| < 0,10" | 18,8 % | **< 5 %** |
| Duty p95 khi cua | 1,00 | **< 0,95** |
| \|v\| p50 khi cua (bốn hướng) | 0,36–0,72 | **0,70–0,74** cả bốn hướng |

## 7. Giới hạn

- **Chưa kiểm trên xe.** Toàn bộ số đo là của log RT38 `(39)`; RT39 mới qua host test và mô phỏng.
- Cua chậm hơn RT38 3 cm/s (72 thay vì 75) — chủ ý, để bánh ngoài còn dự trữ duty.
- Bất đối xứng bánh trái (~11 % duty) là **cơ khí**; nếu sau RT39 hướng lùi-phải vẫn ì, cần kiểm bạc
  đạn/belt/motor trái chứ không chỉnh gain.
- Mức 90 vẫn cần pack ≥ 12,0 V; ở 11,2 V governor sẽ ghìm và ca mô phỏng @90 vẫn ngã như RT38.
