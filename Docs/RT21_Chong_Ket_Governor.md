# R12 RT21-ANTISTALL / NET20 — chống kẹt tham chiếu ở tốc độ thấp

Bản dựng 11/11/2026: STM32 `BAL-20261111-R12-8DIR-RT21-ANTISTALL`, ESP32 `TWIP-WEB-20261111-BAL-R12-8DIR-RT21-ANTISTALL-NET20`. Nguồn: ba log RT20 tại 80 cm/s. **Chưa nạp xe.** Artifact: `.build/balance_release_r12_rt21_antistall_20261111/` kèm `SHA256.json`, `SOURCE_SHA256.json`.

## Vì sao chọn 80 vẫn chỉ chạy 50–60

Đo trên ba log RT20, trong các lượt **giữ tiến ở preset 80**:

| | L3 | L4 | L5 |
|---|---:|---:|---:|
| `v_ref` < 0,60 m/s | 54,7 % | 64,0 % | 51,3 % |
| `v_ref` < 0,30 m/s | 33,7 % | 44,8 % | 26,1 % |
| Cờ governor (16384) bật | 48,2 % | 58,8 % | 65,3 % |

Tỉ lệ kích hoạt từng điều kiện của `pressure` trên các mẫu đang tăng tốc:

| Điều kiện | Tỉ lệ |
|---|---:|
| `pitch + 0,1·gyro > 5°` | 28–32 % |
| `v_ref − v > 0,12 m/s` | 13–22 % |
| `duty > 0,90` | 0–0,2 % |

Hai hệ quả: tham chiếu bị đóng băng ở 0,53/0,60/0,73, và **deadlock** — L4 có 1,64 s liên tục `v_ref` kẹt ở 0,12 m/s trong khi xe đứng yên, vì `v_ref−v = 0,12` chạm ngưỡng còn điều kiện nhả đòi `|v_ref−v| < 0,08`.

## Thay đổi (phương án A)

`balance_governor.h` chỉ sửa hai điểm, giữ nguyên toàn bộ phần còn lại:

1. **Trễ bám tốc khi đang tăng tốc không còn là bằng chứng thiếu dự trữ.** Ngưỡng `.12f` chỉ áp dụng khi tham chiếu đang giữ/phanh; khi đang tăng tốc ngưỡng là `.30f` (chỉ chặn trường hợp bánh chạy ngược rất mạnh).
2. **Chống deadlock**: nếu `limited` mà xe gần như đứng yên (`|v| < 0,05`) và tham chiếu dưới `0,15`, cho tham chiếu nhảy tới `0,15` để phá ma sát tĩnh. Không vượt 0,15 nên không đổi bậc bằng chứng phát hiện kẹt (200 ms).

Không đổi: `duty>0,90`, dự báo nghiêng `>5°` (8° khi khởi hành), `retreat` (PWM bão hòa hoặc dự báo >8°), jerk/gia tốc, gain R12, observer, phân bổ lực, PPR/dấu encoder, CF, ARM 60°, REC1, lease, STOP.

## Ảnh hưởng phụ đã biết

- Phát hiện kẹt với tham chiếu rất nhỏ giờ hủy sau **1,0 s** (trước là 1,5 s), vì tham chiếu được nâng lên 0,15 nên bằng chứng chuyển từ 300 ms sang 200 ms. Vẫn hữu hạn và test `test_rt18_launch` đã cập nhật đúng.
- Test `test_rt18_launch` đổi kỳ vọng: khởi hành ngược (lag ~0,12) **không** còn bị coi là thiếu dự trữ; duty cao, nghiêng lớn và lag >0,30 vẫn kích hoạt.
- Góc thân lớn nhất trong bộ biên tăng nhẹ (ca 4: 7,81° → 9,44°; ca 11: 6,35° → 8,22°), vẫn dưới ngưỡng bảo vệ 25° và không có cờ lỗi.

## PWM và giới hạn motor (trả lời câu hỏi "dùng tối đa")

- Ở 0,79 m/s steady: PWM p50 **2 058–2 079 / 2 499 = 82–83 % duty**, p95 86–91 %. Ngoại suy PWM 2 499 ≈ **0,96 m/s**.
- Mô phỏng: đặt 0,90 và 0,95 cho **cùng một quỹ đạo**; trần mô hình ≈ **0,89 m/s**; vận tốc bền chỉ tăng **1,3 cm/s** so với 0,80.
- Bộ xô 1–4 N: 3/12 ca trụ được ở cả 0,80 và 0,90 — giới hạn nằm ở công suất/độ bám, không ở mức đặt.

→ Giữ preset 80. Phần dư 17 % là để giữ thăng bằng, không phải "chưa dùng hết motor".

## Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **20/20** — thêm `test_rt21_antistall`: trước khi sửa FAIL (`peak>.25`), sau khi sửa PASS (bánh kẹt: tham chiếu lên 0,35 rồi vẫn bị hủy ở 0,91 s; ma sát nhả: đạt 0,80) |
| Mô phỏng stress | 12/12, 20 s, không cờ bảo vệ |
| Mô phỏng biên | 12/12 |
| Mô phỏng khởi hành 50 | 12/12, có kiểm tra hướng |
| Build | STM32 81 520 B text; ESP32 1 079 919 B app; build ID có trong cả hai file nhị phân |

## Còn mở

- **Dự báo nghiêng vẫn là nguồn đóng băng lớn nhất (28–32 % mẫu đang tăng tốc).** RT21-A chỉ sửa phần trễ/deadlock. Nếu sau khi nạp mà vẫn còn lượt 50–60, bước tiếp theo là phương án B: nâng ngưỡng dự báo lên 6,5° (cần chạy lại 36 ca + bộ xô + log mới).
- Chưa có log RT21 trên xe.
- Ba log RT20 có **ESP32 reset=brownout** — cần xử lý nguồn (tụ bulk 470–1 000 µF, đo 5 V/3V3 tại ESP32 khi motor đảo chiều).

## Nạp và thử

1. Nạp `STM32_BAL_R12_RT21_ANTISTALL.bin/.hex` và `ESP32_RT21_NET21.merged.bin` (hoặc nạp sketch bằng Arduino IDE). Bản NET21 chỉ sửa đường kết nối/log của ESP32 và trang web — xem [NET21](NET21_Ket_Noi_Ben_Vung.md); hash STM32 không đổi. Bản thử rút ngắn hiệu chuẩn (`QUICKSTART`, 200 mẫu) **đã khôi phục** ngày 14/11 theo yêu cầu — xem [ghi chú](RT21_QUICKSTART_Hieu_Chuan_Nhanh.md).
2. `http://192.168.4.1/ui/r12?v=RT21-NET21`, phải hiện **MAX 80** và `BAL-20261111-...RT21-ANTISTALL`.
3. Sàn trống, mức 50 → 72 → 80. Bài chính: **bấm tiến từ trạng thái đứng yên** — xe phải tăng tốc đều, không đứng im 0,5–1,5 s rồi mới nhích.
4. Chạy 80, quan sát bảng Chi tiết: `vận tốc đặt` phải lên gần 80 chứ không kẹt 50–60.
5. DỪNG, chờ 2 s, tải log gửi lại.
