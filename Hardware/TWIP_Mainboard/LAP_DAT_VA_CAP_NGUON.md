> **Trạng thái:** bản hiện hành **RT29-NOCUT / NET30** — xem [HANDOFF](../../HANDOFF.md).

> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

# Lắp đặt và cấp nguồn carrier TWIP

Đối chiếu ngày 02/11/2026 giữa hướng dẫn đi dây gốc, schematic và PCB hiện hành. Không thay đổi firmware hoặc kết nối điện trong lần kiểm tra này.

## Kết quả kiểm tra

- Chạy lại KiCad ERC: 0 vi phạm.
- Chạy lại DRC: 0 vi phạm, 0 kết nối thiếu, 0 khác biệt schematic–PCB theo kiểm tra parity.
- 80 dòng chân logic trong NET_CONNECTIONS.csv khớp net thực trên PCB, kể cả chân nguồn lặp lại trên module.
- Đã đối chiếu chức năng các nhóm này với Huong_Dan_Di_Day_Toan_Bo_He_Thong.md: pin, buck, 5V, 3V3, UART, I2C, INT1, PWM, encoder và còi.
- PCB có đường đồng trên F.Cu và B.Cu, 23 via chuyển lớp. Đây là PCB hai lớp.

Hồ sơ kiểm tra mới ở `.build/pcb/component_edges/recheck_drc.json` và `recheck_erc.json`. Kết quả CAD không thay thế kiểm tra bo chế tạo thật, sụt nguồn, nhiễu, nhiệt hoặc tải dòng.

## Khác biệt có chủ đích so với hướng dẫn breadboard

- WAGO và ray breadboard được thay bằng nhánh đồng trên PCB. XL4015 chuyển lên carrier.
- C1–C9 là tụ bổ sung. C4/C6 hiện là tụ hóa phân cực 10µF/16V D4/P2, không còn là tụ gốm như bảng cũ. C5/C7/C8/C9 hiện ghi 50V. Xem BOM_THT.md.
- BMI160 vẫn được cấp ở 3V3, CS lên 3V3, SA0 xuống GND; VIN/INT2/OCS/SCX/SDX không nối chức năng.
- Có hai chân VM và hai chân VCC trên driver; các chân cùng tên được nối cùng rail tương ứng.
- GND chung hai mặt và đường hồi công suất riêng; không phải star-ground một điểm tuyệt đối. Chưa có đo xác nhận dòng motor không gây nhiễu logic.
- Phần mô tả phiên bản firmware và cách đọc IMU trong hướng dẫn gốc là ghi chép lịch sử; không dùng để xác nhận phiên bản thực sự đã nạp.

## Kết nối bên ngoài

```text
Pin LiPo 3S
  (+) ── jack XT tương thích ── [cầu chì/công tắc ngoài nếu lắp] ── J1.1 BAT+
  (−) ── jack XT tương thích ─────────────────────────────────── J1.2 GND

PCB: J1 BAT+ ──┬── VM driver A4950
               └── XL4015 IN+
     XL4015 OUT+ đã chỉnh 5,0V ── ESP32 5V, STM32 5V, driver VCC
     STM32 3V3 ── BMI160 3V3/CS và VCC hai encoder
```

J1 là footprint đầu nối 2 chân bước 5,08mm, lỗ 1,3mm; không phải footprint XT60. Dùng dây đuôi jack phù hợp với pin nối qua đầu nối J1 đúng kích thước và đủ dòng, hoặc hàn dây với biện pháp chống kéo phù hợp. Không suy ra dòng định mức từ bước chân. Không nối pin vào chân 5V, 3V3 hay OUT+ buck. Carrier hiện chưa có linh kiện bảo vệ ngược cực/cầu chì/công tắc riêng trong schematic.

Nhìn từ mặt lắp linh kiện theo bản vẽ chuẩn: J1.1 ở (79,96;108)mm, J1.2 ở (85,04;108)mm. Ưu tiên nhãn BAT+/GND và số chân, không chỉ dựa vào trái/phải vì mặt dưới bị đảo chiều.

J2 nối motor/encoder trái, J3 nối motor/encoder phải. Cả hai cùng thứ tự:

| Chân | Dây theo bộ motor hiện tại | Chức năng |
|---|---|---|
| 1 | Đỏ | M1 motor |
| 2 | Đen | GND encoder |
| 3 | Vàng | Encoder A |
| 4 | Xanh lá | Encoder B |
| 5 | Xanh dương | Nguồn encoder 3,3V |
| 6 | Trắng | M2 motor |

BZ1.1 nối cực (+) còi, BZ1.2 nối cực (−). Còi được PB12 điều khiển trực tiếp theo cấu hình cũ; chưa xác minh dòng còi để chứng nhận tải GPIO.

UART đã nằm trên PCB: STM32 PB6 TX → ESP32 GPIO18 RX; ESP32 GPIO17 TX → STM32 PB7 RX, chung GND, 115200 baud. Không cần dây UART hoặc dây nguồn WAGO ngoài.

## Trước khi cắm pin lần đầu

1. Chế tạo đúng cả hai lớp đồng và nối đủ via. PDF đối chiếu linh kiện không phải bản đường đồng để ăn mòn. Nếu tự làm không mạ lỗ, mọi via và các pad cần chuyển lớp phải có kết nối điện hai mặt thực sự; chỉ hàn chân ở mặt dưới có thể chưa đủ.
2. Chỉnh XL4015 ngoài carrier về 5,0V bằng đồng hồ trước khi hàn nối OUT vào rail cấp MCU. Không cắm pin vào hệ thống đã lắp MCU rồi mới dò chỉnh biến trở.
3. Tắt nguồn; kiểm cực J1, chân module, cực tụ và thông mạch các đường cần nối. Kiểm tra chập nguồn; tụ nạp làm số đo điện trở thay đổi nên không chỉ dựa một tiếng bíp.
4. Chưa cắm motor, cấp nguồn có giới hạn dòng nếu có. Đo lại 5V tại cả ESP32/STM32 và 3V3 tại BMI160/encoder. Không cấp đồng thời USB và pin nếu chưa xác nhận mạch chống cấp ngược của đúng devkit.
5. Sau khi nguồn đạt, nối motor, kê hai bánh quay tự do và thử riêng từng bánh để xác minh chiều, encoder, đồng thời theo dõi reset/sụt nguồn. Nếu firmware không có chức năng thử bánh, dùng quy trình thử motor riêng trước khi ARM trên sàn; không bấm cân bằng với bánh kê cao để thay bài thử motor.

## Mở web và chạy xe

Điều kiện: hai module đã được nạp firmware tương thích của project. Carrier không tự nạp firmware.

1. Cắm pin: ESP32 và STM32 được cấp nguồn. Xe không tự ARM chỉ vì cắm pin.
2. Kết nối Wi-Fi `TWIP_RACER_S3`, mật khẩu `12345678` theo source ESP32 hiện tại.
3. Mở `http://192.168.4.1/`. Mạng này không có Internet; giữ kết nối Wi-Fi với xe.
4. Kiểm tra dữ liệu STM32 cập nhật; Wi-Fi kết nối đơn thuần chưa chứng minh UART/STM32 hoạt động.
5. Nếu đo lại gyro, cố định thân xe yên trong 5 giây, không cần ép đúng góc 0° để đo bias gyro.
6. Đặt bánh trên sàn, giữ thân xe, chọn xác nhận `Trên nền`; chờ giao diện xác nhận đủ điều kiện rồi bấm `BẬT CÂN BẰNG`, thả xe theo quy trình đã thử.
7. Chọn mức tốc độ thấp trước; giữ nút hướng để chạy, thả để kết thúc lệnh chạy và trở về giữ cân bằng. `DỪNG CHẠY` khác `DỪNG MOTOR`: dừng motor sẽ ngắt tác động cân bằng, cần đỡ xe.

Sau chuyển từ breadboard sang PCB vẫn phải thử nguồn tải và kiểm dấu/hướng cảm biến. PCB không tự đảm bảo giữ nguyên chất lượng điều khiển chỉ vì netlist giống nhau.
