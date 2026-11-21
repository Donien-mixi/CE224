# RT37 — Xuất phát mượt: nâng ngưỡng nghiêng của governor

> **RT38 (18/11) kế tiếp:** sửa khựng khi đổi hướng và trôi ở 90 cm/s — xem `RT38_Chuyen_Huong_Lien_Tuc.md`. Các sửa RT37 trong tài liệu này vẫn giữ.

Log `twip_balance_R12 (2) (5).txt`, build **`BAL-20261118-R12-8DIR-RT36-SYM`** / gateway `…-NET36`,
`reset=power_on`, `crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0`, `drops=0`, 7 699 mẫu / 154,0 s.
Người vận hành báo: **ngập ngừng lúc xuất phát và khi tăng tốc**, không cần xuất phát nhanh, chỉ cần
mượt và đúng tốc độ.

## 1. Kiểm tra tính realtime trước (câu hỏi “luồng thiết kế có đúng realtime không?”)

| Đại lượng | Đo được | Ý nghĩa |
|---|---:|---|
| Chu kỳ vòng 200 Hz (`dt_us`) | p50 **4 999**, min 4 996, max **5 009 µs** | jitter ±9 µs trên 5 000 µs (0,2 %) |
| Thời gian chạy 1 tick (`execution_us`) | p50 **642**, p95 **648**, p99 665, max **670 µs** | 13 % ngân sách 5 ms; không tick nào > 3 000 µs |
| Nhịp vòng ngoài 4–6 ms | không có mẫu nào vi phạm | không có `fault 64` |
| `drops` (queue telemetry) | **0** | ISR không bị chặn vì telemetry |
| Tuổi lệnh điều khiển | p50 **13 ms**, p95 **27 ms**, không mẫu > 350 ms | lease 350 ms không hụt |
| Reset gateway | `power_on` | không brownout |

**Kết luận:** vòng điều khiển chạy đúng realtime; hiện tượng ngập ngừng **không** phải do trễ nhịp hay
mất khung lệnh. Nó nằm trong chính sách giới hạn gia tốc của governor.

## 2. Đo hiện tượng

Mỗi cú xuất phát (bắt đầu từ `|v_ref| < 0,02`), 2 s đầu:

| Đại lượng | Kết quả |
|---|---:|
| Thời gian đạt 90 % mốc (0,90) | **2,6–5,5 s** (thiết kế 0,70 m/s² phải ~1,5 s) |
| Governor `limited` (cờ 16384) | **25–44 %** số mẫu |
| Pitch cao nhất | **7–12°** (tư thế trung tính ~2°) |
| `v_ref` sau 2 s | chỉ **0,28–0,74 m/s** |
| Ramp có đoạn phẳng > 0,3 s | 1–4 đoạn mỗi cú |

Chuỗi điển hình (t = 477,4 s): `v_ref` 0,20 → 0,27 trong 0,45 s (0,16 m/s²), rồi bị cắt, rồi lại tăng —
đúng kiểu “đi–dừng–đi”.

## 3. Nguyên nhân

`BalanceGovernor_Step` bật `pressure` khi **dự đoán nghiêng** vượt ngưỡng:

```
predicted = hướng · (pitch − equilibrium + 0,1·gyro)
ngưỡng cũ: 9° khi đang chạy, 11° khi xuất phát
```

Xe này **ngả 7–12° trong 2 s đầu của mọi cú xuất phát** (số đo trên), tức ngưỡng 9°/11° nằm ngay
trong dải làm việc bình thường. Khi `pressure` bật, gia tốc bị kẹp về `BALANCE_GOV_CRAWL = 0,06 m/s²`;
điều kiện xoá (`duty < 0,85` **và** `|lag| < 0,08` trong 300 ms) không đạt trong lúc đang tăng tốc, nên
pressure giữ hàng trăm ms rồi mới nhả — ramp bị băm thành từng đoạn.

## 4. Đã thử ba hướng, chọn hướng không làm xấu mô phỏng

| Phương án | Mô phỏng `--stress` @MAX 90 | Kết luận |
|---|---:|---|
| Giảm gia tốc mồi 0,70 → **0,45** m/s² | **9/12** | loại: thêm 2 ca ngã (pin yếu/motor yếu) |
| Giữ 0,45 + nâng crawl 0,06 → 0,12 | 10/12 | loại |
| Giữ 0,70 + crawl 0,12 | 8/12 | loại |
| **Giữ nguyên ramp + nâng ngưỡng nghiêng 10°/13°** | **11/12** | **chọn** — bằng đúng mức RT35/RT36, chỉ còn ca pin 11,2 V |

Thay đổi chốt (chỉ 2 hằng số, không đổi luật R12):

| Tệp | Cũ | Mới |
|---|---|---|
| [`balance_governor.h`](../Xe_Can_Bang/Core/Inc/balance_governor.h) `BALANCE_GOV_LEAN_DEG` | 9,0° | **10,0°** |
| `BALANCE_GOV_LEAN_LAUNCH_DEG` | 11,0° | **13,0°** |

Biên an toàn còn lại: cắt theo góc 25° (50° khi REC1), taper cua bắt đầu giảm quyền từ 8° vượt trung
tính và về 0 ở 12°, cắt theo duty > 0,90 và lag > 0,30/0,12 vẫn giữ nguyên. Gia tốc mồi, crawl, phanh
nhả, trần cua, neo mốc… **không đổi** so với RT36.

## 5. Kiểm chứng

- **37/37 host test pass**; test mới `test_rt37_smooth`: nghiêng 13,2° khi xuất phát (lệch 11,2°) **không**
  còn áp lực, 15,5° (lệch 13,5°) vẫn ghim; đang chạy 12,0° (lệch 10,0°) không ghim, 12,5° (lệch 10,5°)
  ghim; probe ngưỡng cũ 9/11 **phải fail**.
- Ba test cũ cập nhật theo ngưỡng mới: `test_balance_governor` (9° + gyro 15), `test_rt16_supervisor`
  (14° xuất phát), `test_rt18_launch` (14°).
- Mô phỏng sau sửa: **12/12** @MAX 80, **12/12** biên, **12/12** khởi hành 50, **11/12** @MAX 90
  (chỉ ca pin 11,2 V — như RT35/RT36, không phát sinh ca mới).
- Build: STM32 83 160 B text / 0 lỗi 0 cảnh báo; ESP32 1 090 907 B. Hash trong
  ``Firmware/RT37_SMOOTH_20261118/``.

## 6. Chưa kiểm trên xe

Số ở mục 2 là log **RT36**; RT37 mới qua host test + mô phỏng. Cần một lượt đo lại để xác nhận:
`limited` trong 2 s đầu giảm dưới ~10 %, thời gian đạt 0,90 còn 1,5–2,5 s, không còn đoạn phẳng > 0,3 s,
và tốc độ đích vẫn đúng 0,90 (pin ≥ 12,0 V) mà không ngã.

## 7. Nạp

`Firmware/RT37_SMOOTH_20261118/nap_rt37.bat` — **nạp cả hai board**. Banner `RT37-SMOOTH-NET37`.
