# Tổng quan đồ án TWIP · 08/11/2026

TWIP là xe hai bánh tự cân bằng. STM32 đọc BMI160 và encoder hai bánh, chạy vòng điều khiển và điều khiển motor qua A4950. ESP32-S3 cung cấp web điều khiển, hiển thị trạng thái và tải log. Scilab mô phỏng cơ khí và hành vi điều khiển. KiCad chứa thiết kế bo chuyển tiếp để lắp các module.

| Mảng | Đã làm tới đâu | Việc còn mở |
|---|---|---|
| Giai đoạn 1 · Cân bằng | Nền R12 tự cân bằng và đã được người vận hành thử dựng từ góc 60°. Mã có bù lệch bánh, giữ hướng và phục hồi sau nhiễu trong giới hạn. | Đo lại độ trôi, xoay và khả năng phục hồi với cấu hình pin hiện tại. |
| Giai đoạn 2 · Điều khiển web | Đã có điều khiển giữ tay 8 hướng, ba mức đặt 50/72/80 cm/s, nhả để phanh và cân bằng ở mốc mới, STOP, giám sát kết nối và log. RT19 chốt mốc hướng sau quay, chốt lại mốc vị trí sau phanh và đổi mức cao nhất xuống 80 cm/s; RT18 giữ phần mở cổng sẵn sàng. | Thu log RT19 trên xe thật. Kiểm tra hai chiều ở 50 trước, rồi 72/95. Mức 95 là tốc độ đặt. |
| Cơ khí và đấu dây | Xe dùng khung mica, hai GA25, BMI160 và LiPo 3S. Pin vẫn ở tầng dưới, kê thêm 10 mm. Motor trái đỏ vào AOUT2 và trắng vào AOUT1. | Cân lại nếu cần tính khối lượng miếng kê. Đối chiếu với [thông số mô hình](Thong_So_Xe_Hien_Tai.md) và [hướng dẫn đấu dây](../Huong_Dan_Di_Day_Toan_Bo_He_Thong.md). |
| Mô phỏng | Đã tính phần kê pin 10 mm. Báo cáo RT19 ghi 12/12 ca thường, 12/12 ca khởi hành 50 và 6/6 ca biên qua; chưa chạy lại bộ xô1–4N. | Dùng log thật để xác nhận đáp ứng và giới hạn mô hình. Giữ điều kiện từng lượt khi so sánh. |
| PCB | Bản local một mặt đồng B.Cu, 160 × 65 mm, đủ năm module, BMI160 cách tâm bo 0,85 mm, cần 31 đoạn dây cầu. ERC, DRC và đối chiếu schematic đạt theo bản lắp có dây. | Kiểm tra lắp, thông mạch, nguồn, nhiệt và nhiễu khi có bo thật. |

## Các bản cần phân biệt

- **Workspace hiện tại** dùng STM32 `BAL-20261108-R12-8DIR-RT21-ANTISTALL` và ESP32 `TWIP-WEB-20261108-BAL-R12-8DIR-RT21-ANTISTALL-NET18`. Đã build cả hai và chạy 11 bộ kiểm thử trên máy. Chưa có log RT18 trên xe trong hồ sơ hiện tại. [Báo cáo và cách thử RT18](RT18_Khoi_Hanh_Va_Chong_Ket_Nham.md)
- **Git đã ghi nhận tại workspace** có `main` ở `b95cd71` (RT8/NET5) và `pcb` ở `a65f757` (PCB đời trước). Cần phân biệt commit bàn giao với mã và PCB đang chỉnh trong workspace.

## Điểm vào khi tiếp nhận

1. [HANDOFF](../HANDOFF.md) để biết trạng thái phiên bản và việc cần thử tiếp.
2. [Firmware STM32](../Xe_Can_Bang) và [firmware ESP32-S3](../ESP32_S3_Gateway) để build và nạp đúng cặp. Sau khi nối Wi-Fi xe, mở [web RT21/NET21](http://192.168.4.1/ui/r12?v=RT22-SYM-NET23).
3. [Hướng dẫn mô phỏng](../Simulation/README.md) và [project KiCad](../Hardware/TWIP_Mainboard/TWIP_Mainboard.kicad_pro) để kiểm tra mô hình và thiết kế PCB.
4. [Roadmap](Roadmap_Do_An.md) để xem phương pháp và các mốc. Các mục lịch sử giữ thông số và kết quả của đúng phiên bản tại thời điểm thử.
