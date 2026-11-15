# Xe hai bánh tự cân bằng (TWIP) — STM32F411 + BMI160 + ESP32-S3

Đồ án điều khiển xe hai bánh tự cân bằng: STM32 giữ thăng bằng và chạy quỹ đạo ở 200 Hz,
ESP32-S3 làm gateway Wi-Fi có trang điều khiển và ghi log, Scilab mô phỏng lại đúng phần
luật điều khiển đang nạp trên xe.

**Bản phát hành hiện hành: RT29-NOCUT / NET30 (15/11/2026).**
Firmware đã đóng gói kèm hash trong [`Firmware/RT29_NOCUT_20261115/`](Firmware/RT29_NOCUT_20261115/),
trạng thái kỹ thuật và việc còn mở ở [`HANDOFF.md`](HANDOFF.md).

## 1. Kiến trúc

| Khối | Nội dung |
|---|---|
| STM32F411CEU6 | Vòng điều khiển 200 Hz (TIM4). Đọc BMI160 qua I2C, hai encoder GA25, xuất PWM cho hai driver A4950. Luật lực R12 (14 trạng thái quan sát, K = 9.1508 / 6.1289 / 14.3279 / 1.1605). Ba mức 0,50 / 0,72 / 0,80 m/s, 8 hướng, quay tại chỗ 1,3–2,4 rad/s. |
| Lớp giám sát | `balance_startup` (dựng xe từ 60°), `balance_recovery` (REC1), `balance_governor` (dự phòng mô-men), `balance_interlock` (trần tốc độ), `balance_motion` / `balance_turn` (quỹ đạo, mốc vị trí, mốc hướng). Không đổi gain/observer/phân bổ lực R12. |
| ESP32-S3 | SoftAP `TWIP_RACER_S3` / `12345678`, HTTP :80, WebSocket :81, UART1 115200 (GPIO18/17). Trang `/ui/r12`, log 122 byte/khung, 40 cột, 50 Hz, lưu RAM và vùng Flash. |
| Scilab + GCC | Mô phỏng dùng **chính header C của firmware** (`Simulation/current`, biên dịch native) nên không có bản sao luật điều khiển. |
| KiCad | Bo chuyển tiếp 160 × 65 mm, một mặt đồng B.Cu, 31 dây cầu. |

## 2. Firmware phát hành

| Board | Build ID | Tệp | SHA-256 (rút gọn) |
|---|---|---|---|
| STM32 | `BAL-20261115-R12-8DIR-RT29-NOCUT` | `STM32_BAL_R12_RT29_NOCUT.bin` (+`.hex`, `.elf`) | `16effa542e76975a…` |
| ESP32-S3 | `TWIP-WEB-20261115-BAL-R12-8DIR-RT29-NOCUT-NET30` | `ESP32_RT29_NOCUT_NET30.app.bin` (+ bootloader, partitions, boot_app0) | `0581361f1b797e00…` |

Hash đầy đủ ở `Firmware/RT29_NOCUT_20261115/SHA256.json`; hash của **12 tệp nguồn đã sinh ra
đúng cặp binary này** ở `SOURCE_SHA256.json` trong cùng thư mục. Hai board phải nạp cùng bản:
gateway đối chiếu build ID với STM32 và từ chối điều khiển nếu lệch ngày/bản.

Cách nạp và kiểm hash: xem [`Firmware/RT29_NOCUT_20261115/README.md`](Firmware/RT29_NOCUT_20261115/README.md).

## 3. Cấu trúc repo

| Thư mục | Nội dung |
|---|---|
| `Xe_Can_Bang/` | Project STM32CubeIDE. Luật điều khiển và giám sát ở `Core/Inc/balance_*.h`, FSM và vòng 200 Hz ở `Core/Src/robot_fsm.c`. |
| `ESP32_S3_Gateway/` | Sketch Arduino + trang web (`dashboard_html.h`), giao thức UART (`balance_wire.h`), phiên điều khiển, log RAM/Flash. |
| `Simulation/` | Mô hình Scilab, harness native dùng chung header firmware, bộ 36 ca kiểm tra. |
| `tools/` | Host test (`test_*.cjs`), script build/mô phỏng, tải log cứu hộ. `tools/legacy/` giữ test của các bản RT8/RT9 đã bỏ. |
| `Firmware/` | Binary phát hành + hash + hướng dẫn nạp. |
| `Hardware/` | Schematic, PCB KiCad, bo chuyển tiếp. |
| `Docs/` | Báo cáo từng vòng RT10→RT29, thiết kế, thông số xe, bảng mã lỗi tự dừng. |

## 4. Chạy nhanh

```powershell
# Kiểm thử trên máy (cần Node.js; các test biên dịch C cần GCC tại C:/msys64/ucrt64/bin)
Get-ChildItem tools/test_*.cjs | ForEach-Object { node $_.FullName }

# Mô phỏng native: 12 ca tốc độ / 12 ca biên / 12 ca khởi hành 50 cm/s, mỗi ca 20 s
node tools/run_governor_sim.cjs --stress
node tools/run_governor_sim.cjs --stress --edge
node tools/run_governor_sim.cjs --stress --launch --launch50
```

- **STM32:** mở `Xe_Can_Bang/` bằng STM32CubeIDE, Build rồi nạp ST-LINK (hoặc
  `tools/build_stm32.ps1` khi đã có cấu hình `Debug/`).
- **ESP32:** mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` bằng Arduino IDE, cấu hình board theo
  [`ESP32_S3_Gateway/README.md`](ESP32_S3_Gateway/README.md).
- **Scilab:** chạy `Simulation/build_native.ps1` rồi Execute `robot_can_bang_pid_1.sce` ở gốc repo.
- **Trên xe:** nối Wi-Fi `TWIP_RACER_S3`, mở `http://192.168.4.1/ui/r12`, đo lại gyro khi xe yên,
  xác nhận xe đặt trên nền rồi bật cân bằng. Dừng xe rồi chờ ~2 s trước khi tải log.

## 5. Đã kiểm chứng

| Hạng mục | Kết quả | Điều kiện |
|---|---|---|
| Host test | **30/30 pass** | Node.js 24, GCC 13 (msys2), mỗi test tự biên dịch phần C tương ứng |
| Mô phỏng native | **36/36 ca đủ 20/20 s, không fault** | 12 ca tốc độ + 12 ca biên + 12 ca khởi hành 50 cm/s |
| Nguồn ↔ binary | 12/12 tệp khớp `SOURCE_SHA256.json` | Hash tính lại tại máy này, 15/11/2026 |
| Log xe thật | Dùng để sửa RT22 → RT29 | Log gần nhất `(12)(3)` chạy bản RT28: 3 lần tự ngắt oan `fault 4096`, đã sửa ở RT29 |

## 6. Giới hạn đã biết

- Trần tốc độ thực tế khoảng **0,9–1,0 m/s** do điện áp pack và sức phản điện động; trên 0,9 m/s
  lực còn dùng được chỉ khoảng ±0,2 N nên cú đẩy rất mạnh vẫn có thể làm xe ngã. Muốn nới cần
  đổi pack 4S hoặc motor/giảm tốc, kèm cập nhật `CAT_VNL` và `CAT_PARAM[2]/[4]`.
- Chưa có log xe cho bản RT29 (phần sửa chỉ là miễn hạn 4 s cho phanh do nhiễu).
- Firmware không có watchdog độc lập: nếu MCU treo, TIM1 vẫn giữ duty cuối.
- Mô hình mô phỏng chưa nhận dạng đầy đủ tải, ma sát, trượt; kết quả 36 ca không chứng minh
  phục hồi với mọi cú va.

## 7. Tài liệu

- [`HANDOFF.md`](HANDOFF.md) — trạng thái từng bản, số đo và việc còn mở.
- [`Docs/Tong_Quan_Do_An.md`](Docs/Tong_Quan_Do_An.md) — tổng quan đồ án.
- [`Docs/Bang_Ma_Loi_Tu_Dong_Dung.md`](Docs/Bang_Ma_Loi_Tu_Dong_Dung.md) — mọi trường hợp firmware tự cắt motor.
- [`Docs/Thuat_Toan_Dieu_Khien.md`](Docs/Thuat_Toan_Dieu_Khien.md) — luật điều khiển và các lớp giám sát.
- [`Docs/Thong_So_Xe_Hien_Tai.md`](Docs/Thong_So_Xe_Hien_Tai.md) — thông số cơ khí, điện, encoder.
- [`Docs/RT29_Tu_Ngat_4096.md`](Docs/RT29_Tu_Ngat_4096.md) — bản sửa mới nhất; các vòng trước ở `Docs/RT*.md`.
- [`Simulation/README.md`](Simulation/README.md), [`Hardware/README.md`](Hardware/README.md) — hướng dẫn riêng từng phần.

Log xe thô, dữ liệu đo và script phân tích từng vòng không đưa vào repo; số liệu dùng trong báo cáo
đã chép lại trong `Docs/` và `HANDOFF.md`.
