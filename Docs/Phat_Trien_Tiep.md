> **Trạng thái:** bản hiện hành **STM32 RT34-CORNER + ESP32 NET35** (18/11/2026) — xem [tổng quan](Tong_Quan_Do_An.md) và [HANDOFF](../HANDOFF.md).

# Phát triển tiếp sau bản chốt RT34

Bản chốt đã có: cân bằng R12, ba mức 50/80/90 cm/s, 8 hướng, giữ vị trí/hướng, REC1, bù bánh trái,
trần tốc độ do đường, gateway ổn định. Tài liệu này chỉ liệt kê **việc còn lại** và **hướng mở rộng**,
kèm cách kiểm để không phải suy đoán.

## 1. Việc ưu tiên (phần cứng và vận hành)

| # | Việc | Vì sao | Cách kiểm |
|---|---|---|---|
| 1 | Bấm “SỬA VÙNG LOG FLASH” một lần khi motor đã dừng | `archive_error=1` trong mọi log gần đây ⇒ log chỉ nằm RAM | Log kế tiếp có `archive_saved > 0` |
| 2 | Thêm 1000 µF low-ESR + 100/10 µF tại ESP32, cấp từ nhánh riêng | Gốc rễ brownout vẫn là nguồn; TX 11 dBm chỉ giảm đỉnh dòng | 10 log liên tiếp `reset=power_on` |
| 3 | Xử lý cơ khí motor/hộp giảm tốc trái | Cua trái vẫn ngả ~5° so với ~1,4° cua phải dù tốc độ quay đã cân | So `\|pitch\|` p95 khi quay trái/phải cùng `w_ref` |
| 4 | Pack 4S nếu muốn > 0,90 m/s | 3S chỉ ~0,96 m/s không tải; 0,90 ở 11,2 V cần duty 0,955 | Đo lại đường đặc tính bánh + chạy lại 36 ca mô phỏng |
| 5 | Làm bo PCB thật | Chưa kiểm nhiễu, nhiệt, tải dòng của bo chuyển tiếp | Đo 5 V/3V3 khi motor đảo chiều trên bo thật |
| 6 | Cân nhắc IWDG | Hiện không có watchdog; MCU treo thì TIM1 giữ duty cuối | Phải chứng minh không cắt oan lúc hiệu chuẩn 5 s |

## 2. Hướng kỹ thuật

- **Nhanh hơn 0,90 m/s** chỉ có nghĩa khi đổi nguồn (4S) hoặc đổi motor/giảm tốc. Đổi pack phải đo lại
  đường đặc tính từng bánh rồi cập nhật `Simulation/config/motor_supply.sce` và
  `motor_pair_r8_20261011.sce`, sau đó chạy lại bộ 36 ca — không sửa gain R12 để “ép” tốc độ.
- **Vừa 0,90 vừa cua gắt** là bất khả thi trên 3S: bánh ngoài cần `v + |w|·0,131` ≈ 1,05 m/s. RT34 đã
  chọn hạ tham chiếu khi cua; muốn bỏ hạ thì phải có motor khoẻ hơn, không phải tăng trần.
- **Bù bánh trái** đang là hằng số 110 PWM đo từ log RT30/RT31. Nếu thay motor, hộp giảm tốc hoặc
  bánh, phải đo lại ba chế độ (tiến/lùi/quay) trước khi giữ nguyên số cũ.
- **Tăng tốc/phanh gắt hơn**: đã ở giới hạn thực nghiệm (đảo chiều 0,85–0,90 m/s từng làm xe ngã).
  Mọi thay đổi phải kèm log có `fault` và so trước/sau, không chỉ cảm nhận.

## 3. Hướng mô phỏng

Mô hình đang thiếu: trượt bánh, tụt áp theo tải, tư thế trung tính +1,5°, và mô hình cua đầy đủ.
Thứ tự nên làm: (1) thêm tụt áp theo dòng để kiểm vùng pin yếu; (2) thêm trượt bánh cho cua MAX;
(3) hiệu chỉnh tiếp xúc ở góc lớn. Sau mỗi lần sửa plant, chạy lại 36 ca và ghi rõ ca nào đổi —
xem [Simulation/README](../Simulation/README.md) và [RT35](RT35_Mo_Phong_Khop_Log.md).

## 4. Quy tắc giữ nguyên

- Không sửa gain/observer/phân bổ lực R12, `CAT_VNL`, khung telemetry 122 B.
- Đổi hành vi phải bump build ID **cả hai board** và mọi literal; `test_build_id_sync` phải PASS.
- Chạy đủ 32 host test; test mới phải có old-constant probe (fail với hằng số cũ).
- Không tự nạp board, không tự push `main` khi chưa được yêu cầu.

## 5. Đọc thêm

[Chức năng hiện hành](Tinh_Nang_Xe_Hien_Hanh.md) · [Thuật toán](Thuat_Toan_Dieu_Khien.md) ·
[Thông số xe](Thong_So_Xe_Hien_Tai.md) · [Mã lỗi](Bang_Ma_Loi_Tu_Dong_Dung.md) ·
[RT34](RT34_Cua_Muot.md) · [RT35](RT35_Mo_Phong_Khop_Log.md) · [HANDOFF](../HANDOFF.md).
