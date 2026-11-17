# Tổng quan đồ án TWIP · 17/11/2026

TWIP là xe hai bánh tự cân bằng. STM32F411 đọc BMI160 và hai encoder GA25, chạy vòng điều khiển
200 Hz và điều khiển hai motor GA25 qua driver A4950. ESP32-S3 làm gateway Wi-Fi: trang điều khiển,
hiển thị trạng thái và ghi log. Scilab mô phỏng lại đúng phần luật điều khiển đang nạp. KiCad chứa
bo chuyển tiếp để lắp các module.

| Mảng | Đã làm tới đâu | Việc còn mở |
|---|---|---|
| Cân bằng | Nền R12 (14 trạng thái quan sát, phân bổ lực hai bánh) tự cân bằng, đã được người vận hành dựng từ 60°. Giữ hướng, giữ vị trí, phục hồi sau nhiễu trong giới hạn. | Đo lại độ trôi khi pin yếu; hành vi ở mép 0,90 m/s. |
| Điều khiển web | 8 hướng, ba mức **50 / 80 / 90 cm/s**, nhả để hãm rồi giữ cân bằng tại mốc mới, STOP, giám sát kết nối, tải log. Bốn hướng lái đã cân nhau; cua ở MAX tự hạ tốc độ để không bão hoà bánh. | Chạy thêm log để đối chiếu; hiện chỉ dùng mức 90 khi pin ≥ 12,0 V. |
| Cơ khí và đấu dây | Khung mica, hai GA25, BMI160, LiPo 3S (12,2 V đầy – 11,2 V cạn), pin kê thêm 10 mm. Motor trái đỏ→AOUT2, trắng→AOUT1. Bánh trái yếu hơn: cần thêm ~110 PWM, đã bù trong firmware. | Cân lại nếu đổi khối lượng; xử lý cơ khí bánh trái nếu muốn hết ngả khi cua trái. |
| Mô phỏng | Dùng chung header với firmware; đường đặc tính bánh fit lại từ log xe thật (lệch 1–2 %); 36 ca kiểm tra 20 s. | Mô hình chưa có trượt bánh, tụt áp theo tải, tư thế trung tính +1,5°. |
| PCB | Bản một mặt đồng B.Cu, 160 × 65 mm, đủ năm module, BMI160 cách tâm bo 0,85 mm, 31 dây cầu; ERC/DRC đạt. | Kiểm tra lắp, thông mạch, nguồn, nhiệt, nhiễu khi có bo thật. |

## Các bản cần phân biệt

- **Bản hiện hành:** STM32 `BAL-20261117-R12-8DIR-RT34-CORNER` + ESP32
  `TWIP-WEB-20261117-BAL-R12-8DIR-RT34-CORNER-NET35`. Binary và hash trong
  [`Firmware/RT34_CORNER_20261117/`](../Firmware/RT34_CORNER_20261117/README.md).
- **Bản dự phòng:** RT29-NOCUT / NET30 (ba mức 50/72/80) trong `Firmware/RT29_NOCUT_20261115/`.
- **Lịch sử phát triển** (RT7 → RT21, các bản nạp cũ, log thô, phân tích từng vòng) nằm trong git
  history của repo, không còn giữ trong cây làm việc.

## Điểm vào khi tiếp nhận

1. [HANDOFF](../HANDOFF.md) — trạng thái, quy tắc vận hành, việc còn mở.
2. [Firmware STM32](../Xe_Can_Bang) và [firmware ESP32-S3](../ESP32_S3_Gateway) để build lại từ
   source, hoặc nạp thẳng binary trong `Firmware/`.
3. [Chức năng hiện hành](Tinh_Nang_Xe_Hien_Hanh.md), [thuật toán](Thuat_Toan_Dieu_Khien.md),
   [thông số xe](Thong_So_Xe_Hien_Tai.md), [bảng mã lỗi](Bang_Ma_Loi_Tu_Dong_Dung.md).
4. [Hướng dẫn mô phỏng](../Simulation/README.md) và [project KiCad](../Hardware/TWIP_Mainboard/TWIP_Mainboard.kicad_pro).
5. [Roadmap](Roadmap_Do_An.md) để xem phương pháp và các mốc.
