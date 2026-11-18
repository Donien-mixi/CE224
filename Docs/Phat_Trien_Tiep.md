> **Trạng thái:** bản hiện hành **STM32 RT36-SYM + ESP32 NET36** (18/11/2026) — xem [tổng quan](Tong_Quan_Do_An.md) và [HANDOFF](../HANDOFF.md).

# Phát triển tiếp sau bản chốt RT36

Bản chốt đã có: cân bằng R12, ba mức 50/80/90 cm/s, 8 hướng, giữ vị trí/hướng, REC1, bù bánh trái,
cua ~0,75 m/s, gateway ổn định. Tài liệu này chỉ liệt kê **việc còn lại** và **hướng mở rộng**,
kèm cách kiểm để không phải suy đoán.

## 1. Việc ưu tiên (phần cứng và vận hành)

| # | Việc | Vì sao | Cách kiểm |
|---|---|---|---|
| 1 | **Chạy log xe cho RT36** | Toàn bộ sửa 18/11 mới kiểm trên host test + mô phỏng | Ma trận 6 điều kiện trong [RT36 §4](RT36_Sua_4_Loi.md) |
| 2 | Bấm “SỬA VÙNG LOG FLASH” một lần khi motor đã dừng | `archive_error=1` trong mọi log gần đây ⇒ log chỉ nằm RAM | Log kế tiếp có `archive_saved > 0` |
| 3 | Thêm 1000 µF low-ESR + 100/10 µF tại ESP32, cấp từ nhánh riêng | Gốc rễ brownout vẫn là nguồn; TX 11 dBm chỉ giảm đỉnh dòng | 10 log liên tiếp `reset=power_on` |
| 4 | Xử lý cơ khí motor/hộp giảm tốc trái | Cua trái vẫn ngả ~5° so với ~1,4° cua phải dù tốc độ quay đã cân | So `\|pitch\|` p95 khi quay trái/phải cùng `w_ref` |
| 5 | Pack 4S nếu muốn > 0,90 m/s | 3S chỉ ~0,96 m/s không tải; 0,90 ở 11,2 V cần duty 0,955 | Đo lại đường đặc tính bánh + chạy lại 36 ca mô phỏng |
| 6 | Làm bo PCB thật | Chưa kiểm nhiễu, nhiệt, tải dòng của bo chuyển tiếp | Đo 5 V/3V3 khi motor đảo chiều trên bo thật |
| 7 | Cân nhắc IWDG | Hiện không có watchdog; MCU treo thì TIM1 giữ duty cuối | Phải chứng minh không cắt oan lúc hiệu chuẩn 5 s |

## 2. Hướng kỹ thuật

- **Nhanh hơn 0,90 m/s** chỉ có nghĩa khi đổi nguồn (4S) hoặc đổi motor/giảm tốc. Đổi pack phải đo lại
  đường đặc tính từng bánh rồi cập nhật `Simulation/config/motor_supply.sce` và
  `motor_pair_r8_20261011.sce`, sau đó chạy lại bộ 36 ca — không sửa gain R12 để “ép” tốc độ.
- **Vừa 0,90 vừa cua gắt** là bất khả thi trên 3S: bánh ngoài cần `v + |w|·0,131`. RT36 đã hạ tham
  chiếu cua xuống 0,749 và giữ hai chiều như nhau; muốn bỏ hạ thì phải có motor khoẻ hơn.
- **Bù bánh trái** đang là hằng số 110 PWM đo từ log RT30/RT31. Nếu thay motor, hộp giảm tốc hoặc
  bánh, phải đo lại ba chế độ (tiến/lùi/quay) trước khi giữ nguyên số cũ.
- **Tăng tốc/phanh gắt hơn**: RT36 đã nâng phanh nhả 0,65 → 0,85 m/s². Vượt nữa phải thử trên xe,
  vì đảo chiều 0,85–0,90 m/s từng làm xe ngã.
- **Giữ vị trí khi đứng yên**: cơ chế nhả mốc RT36 chỉ chạy khi thật đứng yên; nếu log cho thấy xe
  trôi trên dốc, phải thêm phát hiện dốc thay vì giảm phạm vi 15 cm.

## 3. Hướng mô phỏng

Mô hình đang thiếu: trượt bánh, tụt áp theo tải, tư thế trung tính +1,5°, và mô hình cua đầy đủ.
Thứ tự nên làm: (1) thêm tụt áp theo dòng để kiểm vùng pin yếu; (2) thêm trượt bánh cho cua MAX;
(3) dựng bài cua riêng trong harness (hiện chỉ chạy thẳng/khởi hành) để kiểm trần 0,749.
Sau mỗi lần sửa plant, chạy lại 36 ca và ghi rõ ca nào đổi — xem [Simulation/README](../Simulation/README.md)
và [RT35](RT35_Mo_Phong_Khop_Log.md).

## 4. Quy tắc giữ nguyên

- Không sửa gain/observer/phân bổ lực R12, `CAT_VNL`, khung telemetry 122 B.
- Đổi hành vi phải bump build ID **cả hai board** và mọi literal; `test_build_id_sync` phải PASS.
- Chạy đủ 36 host test; test mới phải có old-constant probe (fail với hằng số cũ).
- Không tự nạp board, không tự push `main` khi chưa được yêu cầu.

## 5. Đọc thêm

[Chức năng hiện hành](Tinh_Nang_Xe_Hien_Hanh.md) · [Thuật toán](Thuat_Toan_Dieu_Khien.md) ·
[Thông số xe](Thong_So_Xe_Hien_Tai.md) · [Mã lỗi](Bang_Ma_Loi_Tu_Dong_Dung.md) ·
[RT36](RT36_Sua_4_Loi.md) · [RT34](RT34_Cua_Muot.md) · [RT35](RT35_Mo_Phong_Khop_Log.md) ·
[HANDOFF](../HANDOFF.md).
