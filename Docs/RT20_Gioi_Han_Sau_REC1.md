# R12 RT20-POSTREC / NET19 — giới hạn tốc độ sau REC1 có thời hạn

Bản dựng 08/11/2026: STM32 `BAL-20261108-R12-8DIR-RT20-POSTREC`, ESP32 `TWIP-WEB-20261108-BAL-R12-8DIR-RT20-POSTREC-NET19`. Nguồn: ba log RT19 tại 50/72/80 cm/s. Đã build cả hai; **chưa nạp xe**. Artifact: `.build/balance_release_r12_rt20_postrec_20261108/` kèm `SHA256.json`, `SOURCE_SHA256.json`.

## Kết quả tích cực: preset 80 chạy đúng

| Preset | v_ref dài hạn | Vận tốc thực dài hạn | Đỉnh | Governor ghìm |
|---|---:|---:|---:|---:|
| 50 | 0,500 | 0,501 | 0,624 | 4,3 % |
| 72 | 0,720 | 0,721 | 0,798 | 13,9 % |
| 80 | 0,800 | **0,807** | 0,857 | 23,1 % |

Việc đổi 95 → 80 đã có tác dụng: cả ba mức đều đạt đúng giá trị đặt. Vấn đề "chọn 80 nhưng chỉ 50–60" trong ba log này **không phải do governor**, mà do lỗi được mô tả dưới đây.

## Lỗi: sau REC1, trần 0,5 m/s không bao giờ được nhả

Log `(2) (2)`, phiên 80 cm/s:

| Thời điểm | Sự kiện | Bằng chứng |
|---|---|---|
| 219,17 s | Va chạm nhẹ → REC1 (phase 2) | gyro **190,9°/s**, pitch −7,2° rồi −16,6° |
| 219,33 s | Người dùng **nhả** lệnh (dir −1 → 0) | vẫn trong REC1 |
| 220,47 s | **Bấm tiến lại** (0 → +1) | **vẫn trong REC1** (REC1 hết 220,69) |
| 220,71–227,75 s | REC1 đã xong, cờ 32768 **giữ 7 s** | `v_ref` kẹp **0,500**; thực 0,477–0,55 |

**Nguyên nhân gốc** ([balance_turn.h](../Xe_Can_Bang/Core/Inc/balance_turn.h)): cờ `recovery_limited` chỉ được xóa khi `permission==1` **và** có đổi hướng/đổi quay. Cú bấm lại rơi vào lúc REC1 còn hoạt động (`permission=2/3`), nên bằng chứng "thao tác mới" bị tiêu mất; khi REC1 kết thúc thì `s->requested` đã bằng hướng đang giữ nên không còn "đổi hướng" để xóa. Xe giữ trần 0,5 m/s vô thời hạn.

## Sửa: trần sau REC1 có thời hạn 1,5 s

- Thêm bộ đếm `post_rec_ticks` vào `BalanceMotion` (có trong ABI native: trạng thái 28 → 29, đầu ra 29 → 30; `twip_motion_reference` giữ `valid` ở vị trí 30).
- Trong `balance_turn.h`: khi REC1 đang hoạt động, bộ đếm về 0; khi REC1 kết thúc mà vẫn còn `recovery_limited`, đếm **300 tick (1,5 s)** rồi tự xóa cờ. Đổi hướng/đổi quay trong trạng thái thường vẫn xóa ngay lập tức như cũ.
- Không đổi gain, observer, phân bổ lực, governor, CF, encoder/PPR/dấu, ARM 60°, REC1 limits hay lease.

Hành vi mới: sau va chạm, REC1 vẫn giữ 0,25 m/s trong lúc phục hồi; 1,5 s sau khi phục hồi xong, mức đã chọn (80) trở lại **dù vẫn đang giữ nút** — không cần nhả rồi bấm lại.

## Encoder: hoạt động đúng, không phải nguồn của "lùi/tiến nhẹ"

| Kiểm tra | Kết quả |
|---|---|
| `x` từ tick vs telemetry | lệch max **7,7e-7 m** |
| Vận tốc lọc vs vận tốc tính lại từ Δtick | tương quan **0,9999 / 0,9999 / 0,9897**, trễ 0 mẫu |
| Bước tick lớn nhất trong 20 ms | 54 / 70 / 76 tick ≈ 0,6–0,86 m/s — khớp tốc độ thật, không nhảy |
| Khi bánh yên (|v|<0,02, ≥1 s) | `x` dao động p-p **0,22–0,75 cm** |

Hiện tượng xe "lùi và tiến nhẹ" sau khi nhả lệnh gồm: quán tính phanh (40–80 cm từ 0,8 m/s, lý thuyết ~49 cm) và dao động cân bằng ±3–8 cm ở |v| ≤0,08 m/s tắt dần. Vòng vị trí R12 chỉ tạo ~0,6 N ở 7 cm, sát ma sát tĩnh, nên xe bò rất chậm về mốc. **Không phải lỗi encoder.**

## Wi-Fi: số liệu và khuyến nghị

| Chỉ số | Ba log |
|---|---|
| Tuổi lệnh tại STM32 khi đang giữ | p50 13–14 ms, p95 34–41 ms |
| Mất lệnh khi đang chạy (age>350 ms, dir≠0) | **0 lần** |
| Ngắt lệnh ngắn | 3 đoạn 0,7–1,3 s, đều khi dir=0 (trang bị ẩn) |
| CRC / mất khung / tràn queue / STM32 reset | 0 / 0 / 0 / 0 |
| ESP32 reset | `power_on`, boot không đổi — **không crash** |
| Trước mỗi STOP | pitch 1–2°, v≈0 — xe đang đứng yên |

Tuyến STM32↔ESP32 và bản thân ESP32 ổn định. Điểm yếu là **điện thoại ↔ SoftAP**: khi trang bị ẩn hoặc Wi-Fi điện thoại chập chờn, web ngừng gửi MOVE8 → lease 350 ms hết hạn → xe phanh và đứng (đúng thiết kế). Không log nào có fault 2 (mất nhịp) hay fault 4 (góc); cả ba kết thúc bằng **fault 1 = STOP**, nên trong ba file này STM32 **không tự dừng**.

Khuyến nghị: chưa chuyển hẳn sang Bluetooth. Giữ Wi-Fi cho telemetry/tải log; nếu vẫn chập chờn, thêm BLE làm **kênh điều khiển thứ hai** dùng chung `$MOVE8` (STM32 không phải sửa). Trước mắt: ghi `/network-log` mỗi lượt (`ap_leaves`, `ap_reason`), chọn kênh AP tránh nhiễu, và hiển thị rõ trạng thái "mất kênh → đang phanh".

## Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **19/19** — thêm `test_rt20_post_rec` (trước khi sửa: FAIL đúng chỗ `clear_tick`; sau khi sửa: PASS, nhả ở tick 300 cả khi giữ liên tục và khi nhả–bấm trong REC1) |
| Mô phỏng stress | **12/12** ca 20 s, không cờ bảo vệ |
| Mô phỏng biên | **12/12** |
| Mô phỏng khởi hành 50 | **12/12**, có kiểm tra đã chạy đúng hướng |
| Build | STM32 81 416 B text; ESP32 1 079 415 B app; build ID có trong cả hai file nhị phân |

Hai thay đổi chỉ thuộc công cụ mô phỏng: ABI native mở rộng cho `post_rec_ticks`; `motion_ref` truyền `yaw_mode=1` tường minh (giữ nguyên hành vi cũ — xem "còn mở").

## Còn mở

- **Chưa có log RT20 trên xe.**
- Sim Scilab đang truyền **`yaw_mode=1` cố định** (di sản: trước đây lấy nhầm cờ `valid`), nên mô phỏng **không** kiểm chứng phần giữ hướng của RT19/RT20; phần đó dựa vào host test `test_rt19_hold`. Cần sửa riêng `motion_ref` để dùng `rate_mode` thật rồi chạy lại toàn bộ.
- Bộ xô 1–4 N chưa chạy lại.
- Ba log không có bản ghi `$GSTOP` dù có fault 1; cần kiểm tra lại trên lượt sau (có thể do STOP phát từ một phiên khác hoặc bộ đếm bị reset).
- Governor vẫn ghìm 23 % mẫu ở mức 80 (vẫn đạt 0,807). Nếu muốn mượt hơn nữa mới cần F4/F5.

## Nạp và thử

1. Nạp STM32 (`STM32_BAL_R12_RT20_POSTREC.bin/.hex`) và ESP32 (`ESP32_RT20_NET19.merged.bin` nếu nạp cả phân vùng). Đúng cả hai vì web kiểm tra build ID.
2. `http://192.168.4.1/ui/r12?v=RT20-NET19`, trang phải hiện **MAX 80** và `BAL-20261108-...RT20-POSTREC`.
3. Mức 50 → 72 → 80. Ở mỗi mức: chạy thẳng, nhả, để yên 3 s; sau đó **va nhẹ** (hoặc xô nhẹ) khi đang chạy 80 rồi **giữ tiếp lệnh tiến** — sau tối đa 1,5 s xe phải trở lại ~80 cm/s, không kẹt ở 50–60.
4. Thử cả trường hợp nhả tay trong lúc REC1 rồi bấm lại: vẫn phải về 80 sau 1,5 s.
5. DỪNG, chờ 2 s, tải log.
