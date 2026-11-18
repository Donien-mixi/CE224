> **Trạng thái đồ án · 18/11/2026.** Nền R12 đã được thử cân bằng và dựng từ 60°. Điều khiển web có 8 hướng và log. Workspace đang ở **RT38-CONT / NET38**, đã build, kiểm thử trên máy và push Git. Mô hình tính pin kê thêm 10 mm. PCB một mặt đồng đã qua kiểm tra KiCad với 31 dây cầu. Xem [tổng quan và các việc còn mở](../Docs/Tong_Quan_Do_An.md).

> Các mô tả theo phiên bản cũ bên dưới là tài liệu đối chiếu. Dùng tổng quan phía trên để xác định cấu hình hiện hành.

> Bản hiện hành01/11/2026: PCBcarrier toàn bộ chân xuyên lỗ, bao gồm9tụ. Schematic đã gánfootprintTHT, đi dây lại; ERC/DRC/parity0. Xem TWIP_Mainboard/BOM_THT.md và ROUTING_CHECKS.json. Chưa phê duyệt sản xuất vì các xác nhận cơ khí/tải còn thiếu.

# Sơ đồ nguyên lý xe hai bánh tự cân bằng

Project KiCad **10.0**, sơ đồ revision **1.3**, cập nhật 30/10/2026.

## Mở project

1. Tải hoặc clone repo, giữ toàn bộ thư mục `Hardware`.
2. Mở `TWIP_Mainboard/TWIP_Mainboard.kicad_pro` trong KiCad.
3. Mở `TWIP_Mainboard.kicad_sch` để xem/sửa sơ đồ nguyên lý.

Giữ `TWIP_Modules.kicad_sym` và `sym-lib-table` cùng thư mục project. Thư viện được tham chiếu bằng `${KIPRJMOD}`, không phụ thuộc đường dẫn trên máy tác giả.

## Nội dung và phạm vi

- Năm bo/module có sẵn: STM32F411 Black Pill, ESP32-S3 N16R8, GY-BMI160, Dual A4950 và XL4015.
- Nguồn pin LiPo 3S, nguồn 5 V từ XL4015 và nguồn 3,3 V từ Black Pill.
- UART, I2C, PWM, hai giắc motor/encoder 6 dây và còi hai chân.
- INT1 BMI160 → STM32 PB2; còi (+) → PB12, còi (−) → GND: người dùng xác nhận đã nối trên breadboard.
- C1–C9 là tụ bổ sung để lắp trên PCB mới; breadboard hiện tại chưa có các tụ ngoài này. Tất cả tụ đều được đánh dấu sẽ lắp. Giá trị và vị trí theo tên các khung trong sơ đồ.
- Không thêm đầu nối ST-LINK riêng; dùng chân SWD có sẵn trên Black Pill.

Đây là sơ đồ **kết nối module**, không phải thiết kế lại mạch bên trong các module. Các tên chân logic của symbol module chưa phải số pad vật lý. Chưa gán footprint; cần xác minh đúng phiên bản bo, kích thước, khoảng cách và thứ tự chân trước khi làm PCB. `TWIP_Mainboard.kicad_pcb` đã có bản bố trí P0 150×60mm ngày31/10, nhưng chưa đi dây đồng do footprint/pad map module chưa xác minh. Xem [trạng thái PCB](TWIP_Mainboard/PCB_DRAFT.md); không dùng đặt sản xuất.

Sơ đồ đã kiểm tra bằng KiCad CLI: **0 lỗi, 0 cảnh báo ERC** khi bàn giao. Kết quả ERC không xác nhận kích thước module hoặc khả năng vận hành thực tế. Các tụ bổ sung cần được kiểm chứng với XL4015 và tải motor thực tế khi thử bo.

## Trạng thái firmware đi kèm repo

Commit Hardware ban đầu `d6a14af` chưa có firmware mới. Bản đồng bộ hiện tại của repo là STM32
**RT38-CONT** + ESP32 **NET38** (binary và hash trong `Firmware/RT36_SYM_20261118/`). Sơ đồ vẫn
là revision 1.3; việc đồng bộ source không xác nhận firmware đã nạp trên xe.
