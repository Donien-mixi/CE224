# Bàn giao đồ án TWIP — trạng thái 17/11/2026

> **Bản hiện hành: STM32 `RT34-CORNER` + gateway ESP32 `NET35`.** Ba mức tốc độ **50 / 80 / 90 cm/s**,
> bù lệch bánh trái, trần tốc độ do đường (cua ở MAX 90 tự hạ về ~0,80 m/s), gateway đã sửa lỗi tự
> khởi động lại. Binary + hash: [`Firmware/RT34_CORNER_20261117/`](Firmware/RT34_CORNER_20261117/README.md).

## 1. Trạng thái từng mảng

| Mảng | Hiện trạng | Đã kiểm trên xe? |
|---|---|---|
| Cân bằng R12 | Luật 14 trạng thái, observer, phân bổ lực hai bánh — **đóng băng**, không đổi từ RT8 | Có (nhiều log) |
| Tốc độ | 50 / 80 / 90 cm/s; 90 chỉ khi pin đo **≥ 12,0 V** | Có — log `(1)(5)`, 173 s, 0 ngã |
| Bẻ hướng | Quay tại chỗ 1,3/2,4 rad/s; chéo 0,90/1,15 rad/s; **trần tốc độ do đường** khi cua ở MAX | Có — yaw khớp tham chiếu, cờ 512 còn 19–26 % |
| Chống đẩy / nhiễu | REC1 (50°/1,2 m/s/1 m/4 s), giữ mốc khi bị đẩy (RT28), miễn hạn 4 s cho phanh do nhiễu (RT29) | Có — nhiều log, có cú va ~160 °/s vẫn phục hồi |
| Giám sát an toàn | Interlock 200 ms + 1,45 m/s, cắt theo góc/tốc độ/quãng/I2C/vòng lặp, không tự ARM lại | Có |
| Gateway ESP32 | Web `/ui/r12`, log 122 B/40 cột/50 Hz, SoftAP, TX 11 dBm, **không ghi Flash khi xe đang chạy** | Có — `reset=power_on`, hết brownout |
| Mô phỏng | Dùng chính header firmware; đường đặc tính bánh fit lại từ log `(1)(5)` (lệch 1–2 %) | Không áp dụng |
| Kiểm thử | **32/32 host test**; mô phỏng 36/36 ca khi mức ≤ 80 | — |
| PCB | Bo chuyển tiếp 160 × 65 mm, một mặt đồng B.Cu, 31 dây cầu, ERC/DRC đạt | Chưa có bo thật |

## 2. Nạp firmware

| Bản | Thư mục | Khi nào dùng |
|---|---|---|
| **RT34-CORNER / NET35** (hiện hành) | `Firmware/RT34_CORNER_20261117/` | Chạy bình thường. Nạp **cả hai board** |
| RT29-NOCUT / NET30 (dự phòng) | `Firmware/RT29_NOCUT_20261115/` | Quay về bản 0,80 m/s nếu RT34 có vấn đề |

Nháy đúp `nap_rt34.bat` trong thư mục bản nạp → nhập COM của ESP32. Kiểm sau khi nạp: banner
`RT34-CORNER-NET35`, ba nút **50 / 80 / MAX 90**, nút ĐO LẠI GYRO bấm được.

## 3. Quy tắc vận hành (quan trọng)

1. **Đo pin trước mỗi phiên**: MAX 90 chỉ khi **≥ 12,0 V**; 11,7–12,0 V dùng 50/80; dưới 11,7 V chỉ 50/80.
2. **Giảm tốc rồi mới đảo chiều / buông ga** ở tốc độ cao — hai cú ngã từng ghi nhận đều do đảo chiều
   đột ngột ở 0,85–0,90 m/s (PWM nhảy 100 %, thân quật 18–28°).
3. Sau mỗi lượt: **STOP → chờ ~2 s → tải log** (gateway chỉ ghi Flash khi đã dừng).
4. **Đừng bấm “SỬA VÙNG LOG FLASH” khi xe đang cân bằng** — format 9,9 MB làm ESP32 khởi động lại
   một lần (hành vi đã biết, không phải lỗi).
5. Nếu mất kết nối: STM32 vẫn tự cân bằng, nhưng sau **10 s** không nhận lệnh sẽ dừng motor (bảo vệ
   mất liên lạc). Đừng cố giữ ga trong lúc mất kết nối.

## 4. Việc còn mở (theo thứ tự ưu tiên)

1. **`archive_error=1`** trong mọi log gần đây: vùng log Flash chưa được format lại. Cần bấm
   “SỬA VÙNG LOG FLASH” một lần khi motor đã dừng (xe đứng yên, chỉ ESP32 chạy) rồi kiểm
   `archive_saved` > 0 trong log kế tiếp.
2. **Nguồn 5 V/3V3 tại ESP32**: thêm 1000 µF low-ESR + 100/10 µF, cấp từ nhánh riêng. Brownout đã
   hết nhờ hạ TX xuống 11 dBm, nhưng đây vẫn là gốc rễ phần cứng (xem [RT33](Docs/RT33_On_Dinh_Ket_Noi.md)).
3. **Độ nghiêng khi cua trái** còn lớn hơn cua phải (~5° so với ~1,4°) dù tốc độ quay đã cân — phần
   cơ khí (motor/hộp giảm tốc trái) cần xử lý nếu muốn cân hẳn.
4. **Tốc độ > 0,90 m/s**: cần pack 4S (3S chỉ tới ~0,96 m/s không tải). Xem [RT34](Docs/RT34_Cua_Muot.md).
5. **PCB**: chưa có bo thật để kiểm nhiễu/nhiệt/tải dòng.

## 5. Cấu trúc repo và tài liệu

| Đường dẫn | Nội dung |
|---|---|
| `Xe_Can_Bang/` | Project STM32CubeIDE: luật điều khiển trong `Core/Inc/balance_*.h`, vòng 200 Hz ở `Core/Src/robot_fsm.c` |
| `ESP32_S3_Gateway/` | Sketch Arduino + web + log |
| `Simulation/` | Mô hình Scilab + harness native dùng chung header firmware |
| `tools/` | 32 host test + script mô phỏng/build; `tools/legacy/` giữ test bản RT8/RT9 |
| `Firmware/` | Hai bản nạp: RT34 (hiện hành) và RT29 (dự phòng), kèm SHA256 |
| `Docs/` | Tài liệu thiết kế + báo cáo các vòng RT22 → RT35 (xem [README](README.md#7-tài-liệu)) |
| `Hardware/` | KiCad: schematic, PCB một mặt đồng, thư viện footprint |

Lịch sử phát triển đầy đủ (RT7 → RT21, các bản nạp cũ, log và phân tích từng vòng) nằm trong **git
history** của repo, không còn giữ trong cây làm việc để repo gọn cho báo cáo.
