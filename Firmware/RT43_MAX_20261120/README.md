# Bản chốt RT43 — trần MAX 90 theo duty còn dư (hết ngã ở mốc 90)

Nạp **cả hai board**.

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261120-R12-8DIR-RT43-MAX` |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT43-MAX-NET43` |

## 1. Log RT42 tại nhà (bạn vừa gửi): cái gì đã tốt, cái gì chưa

`twip_balance_R12 (1) (7).txt` — đúng **RT42-RF / NET42**, 6 551 mẫu / **148,6 s**, 4 lần ARM,
`drops=0`, `dt` 4 999–5 000 µs, `exec` p95 653 µs, **không `fault` nào khi `state=3`**.

Dòng `# RADIO` (mới của RT42) chạy đúng: **`ch=6 rssi=-24 phy=11n ws_joins=1 ws_leaves=0 ap_joins=1 ap_leaves=0`**
→ điện thoại **gắn một lần, không rớt lần nào**, dBm −24 (rất mạnh), tốc độ **11n**. Tức là ở nhà,
đường sóng không còn là vấn đề.

### 1.1 Đường lệnh: đã đạt

| Chỉ số | RT42 (log này) | RT41 | RT39 (6 log) |
|---|---:|---:|---:|
| `age` p50 / p95 / p99 / max | 13 / **27** / **40** / **54 ms** | 13 / 29 / 45 / 130 | 13 / 28 / — / 64 |
| Khe 2 gói lệnh khi **giữ liên tục** p50/p95/p99/max | 23 / **34** / 50 / **71 ms** | — | — |
| Số lần khe > 150 ms khi giữ liên tục | **0** | 0 | 0 |
| Rớt mẫu / mất gói | 0 / 0 | 0 / 0 | 0 |

`age` max 54 ms (RT41: 130 ms) — đường lệnh **ổn định hơn cả RT41**, và không có lần nghen nào khi
đang giữ lệnh. **Yêu cầu "truyền nhận tức thời" đã đạt.**

### 1.2 Điều khiển: RT40 vẫn giữ, nhưng **MAX 90 vẫn ngã**

| Chỉ số | RT42 | RT41 | RT39 |
|---|---:|---:|---:|
| Thời gian REC1 | **5,6 %** | 23,5 % | 30,2 % |
| Có lệnh mà \|v\| < 0,10 | 10,5 % | — | 18,8 % |
| Ngã | **2** | 0 | 3 |

Toàn bộ 148 s chỉ chạy **preset 50 và MAX 90** (không có 80). Hai lần ngã đều ở **MAX 90**, và cả hai
đều có cùng dấu hiệu:

| t (s) | Tình huống | Trước khi ngã |
|---:|---|---|
| 144,69 | lùi **thẳng** ở −0,90 m/s | v vọt −0,95 khi tham chiếu −0,90; pwm −2337→**−2499** (ghim); duty p95 **1,00**; rồi bánh trái/phải lệch **−0,91 so với −0,71** → thân quật −55° |
| 235,93 | nhả lệnh tiến ở +0,90 | duty ghim, pitch +48° |

Ở 34 lượt chạy MAX, **duty p95 = 0,90 và có tới 33 % số mẫu ≥ 0,96** ở một lượt. Pack lúc test
~11,5–11,8 V: 0,90 m/s cần 93–96 % duty nên **không còn dự trữ cho vòng cân bằng** — vượt tham chiếu
0,05 m/s là bánh bão hoà, lệch trái/phải (bánh trái vốn cần ~11 % duty nhiều hơn) rồi thân ngã.

**Kết luận:** lỗi truyền nhận đã khắc phục; lỗi còn lại là **mốc 90 vượt khả năng của pack**, không
phải RF, không phải luật cân bằng.

## 2. RT43 sửa gì

Thêm **trần MAX theo duty còn dư ở đường thẳng** (cùng cơ chế với trần cua RT40):

- Đang chạy **MAX**, **không cua**, và có bánh **≥ 96 % duty** → hạ tham chiếu tuyến tính theo hệ số
  0,004/nhịp, **sàn 0,86** (0,90 → **0,774 m/s**); hồi lại khi duty ≤ 90 %.
- **Pack tươi**: 0,90 chỉ cần 88 % duty → hệ số **không bao giờ giảm** → MAX vẫn đạt đúng **0,90**.
- Chỉ tác dụng khi **không cua** nên tốc độ cua (0,719 m/s của RT40) **không đổi**.

| Tệp | Thay đổi |
|---|---|
| [`balance_motion.h`](../../Xe_Can_Bang/Core/Inc/balance_motion.h) | `BalanceMotion_maxhead()` / `_next()` + nhân vào `speed_limit` khi preset MAX |
| [`robot_fsm.c`](../../Xe_Can_Bang/Core/Src/robot_fsm.c) | Cập nhật hệ số mỗi nhịp từ duty đo được |

## 3. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **42/42**; `test_rt43_max`: duty 0,99 → v_ref **0,774**; duty 0,85 → v_ref **0,900**; cua không ảnh hưởng; hết ghim thì hồi; probe `MAXHEAD_FLOOR=1.0` **fail** |
| STM32 | bin 84 268 B, 0 lỗi 0 cảnh báo |
| ESP32 | 1 094 304 B (không đổi hành vi so RT42) |
| Hash | 7 binary + 16 nguồn trong `SHA256.json` / `SOURCE_SHA256.json` |

## 4. Nạp + kiểm

`nap_rt43.bat` (nhập COM ESP32) hoặc menu `Firmware\nap_chon_ban.bat` → **1**, hoặc CubeIDE.
Banner phải là **`RT43-MAX-NET43`**.

Kỳ vọng ở log sau (chạy MAX 90 với pack ~11,6 V):
- `duty` p95 **< 0,96** (trước: 0,90 p95, tới 33 % mẫu ≥ 0,96)
- `|v|` không vượt tham chiếu quá 0,05 m/s; **không ngã**
- Nếu vẫn muốn đúng 0,90 m/s: dùng pack **≥ 12,0 V** (0,90 cần 88 % duty) — khi đó trần này không can thiệp.

## 5. Giới hạn

- **Chưa kiểm trên xe** (mới qua host test + lý thuyết duty).
- Trần này **không sửa được cơ khí**: bánh trái vẫn cần ~11 % duty nhiều hơn bánh phải; nếu log mới
  vẫn thấy lệch trái/phải > 0,15 m/s thì phải kiểm bạc đạn/belt/motor trái.
- Mốc 80 và 50 không bị ảnh hưởng (trần chỉ áp cho preset MAX).
