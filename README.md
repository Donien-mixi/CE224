# Xe hai bánh tự cân bằng (TWIP) — STM32F411 + BMI160 + ESP32-S3

Đồ án điều khiển xe hai bánh tự cân bằng: STM32 giữ thăng bằng và chạy quỹ đạo ở 200 Hz, ESP32-S3
làm gateway Wi-Fi có trang điều khiển và ghi log, Scilab mô phỏng lại đúng phần luật điều khiển
đang nạp trên xe.

**Bản phát hành hiện hành: STM32 `RT46-SLIM` + ESP32 `NET46` (21/11/2026)** — **xoá code chết, không
đổi hành vi điều khiển** so với RT45 (PID cascade + 2 header không dùng): ba mức tốc độ **50 / 80 / 90 cm/s**, bù lệch bánh trái, cua
**0,72 m/s** ở cả hai chiều, chống trôi khi đứng yên, xuất phát mượt, đổi hướng liên tục, không ngập
ngừng khi phanh, không cắt motor khi bị giữ, **UART 921600, tự chọn/đổi kênh Wi-Fi, log mang theo chất
lượng sóng, trần MAX 90 theo duty**.
RT46 nạp bằng IDE từ source (chưa đóng gói); bản đóng gói gần nhất là ``Firmware/RT45_CLEAN_20261121/``
(dùng để quay lui), trước nữa là RT44.
Trạng thái và việc còn mở: [`HANDOFF.md`](HANDOFF.md).

## 1. Kiến trúc

| Khối | Nội dung |
|---|---|
| STM32F411CEU6 | Vòng điều khiển 200 Hz (TIM4). BMI160 qua I2C, hai encoder GA25, PWM cho hai driver A4950. Luật lực R12 (14 trạng thái quan sát, K = 9.1508 / 6.1289 / 14.3279 / 1.1605) — **đóng băng**. Ba mức 0,50 / 0,80 / 0,90 m/s, 8 hướng. |
| Lớp giám sát | `balance_startup` (dựng từ 60°), `balance_recovery` (REC1), `balance_governor` (dự phòng mô-men, **neo mốc chỉ khi bão hoà**, phanh nhả 0,85 m/s²), `balance_interlock` (trần tốc độ), `balance_motion` / `balance_turn` (quỹ đạo, mốc vị trí/hướng, **trần cua 0,87 − \|w\|·0,131 → 0,719 m/s**, **nhả mốc khi đứng yên**), `balance_trim` (**bù lệch bánh trái**). Không đổi gain/observer/phân bổ lực R12. |
| ESP32-S3 | SoftAP `TWIP_RACER_S3` / `12345678`, HTTP :80, WebSocket :81, UART1 115200 (GPIO18/17). Trang `/ui/r12`, log 122 byte/khung, 40 cột, 50 Hz; TX WiFi 11 dBm; **chỉ ghi Flash khi xe đã dừng**. |
| Scilab + GCC | Mô phỏng dùng **chính header C của firmware**; đường đặc tính bánh fit lại từ log xe thật (lệch 1–2 %). |
| KiCad | Bo chuyển tiếp 160 × 65 mm, một mặt đồng B.Cu, 31 dây cầu. |

## 2. Firmware phát hành

| Bản | Board | Build ID | Tệp |
|---|---|---|---|
| **RT46-SLIM / NET46** (bản hiện hành) | STM32 | `BAL-20261121-R12-8DIR-RT46-SLIM` | nạp bằng IDE (Clean → Build → Run) |
| | ESP32-S3 | `TWIP-WEB-20261121-BAL-R12-8DIR-RT46-SLIM-NET46` | nạp bằng Arduino IDE (Upload) |
| RT45-CLEAN / NET45 (quay lui) | STM32 + ESP32 | `BAL-20261121-R12-8DIR-RT45-CLEAN` / `…-NET45` | `Firmware/RT45_CLEAN_20261121/` |
| RT44-TURN / NET44 (bản trước) | STM32 + ESP32 | `BAL-20261120-R12-8DIR-RT44-TURN` / `…-NET44` | `Firmware/RT44_TURN_20261120/` — quay lui |

- Hash đầy đủ: `SHA256.json` trong từng thư mục; hash **20 tệp nguồn đã sinh ra binary**:
  `SOURCE_SHA256.json` (RT45).
- Hai board phải nạp cùng bản: gateway đối chiếu build ID với STM32, lệch là khoá điều khiển.
- Cách nạp: STM32CubeIDE (**Clean** → **Build** → **Run**) và Arduino IDE (**Upload**) — xem
  [`Firmware/README.md`](Firmware/README.md); quay lui bằng `nap_rt45.bat` / `nap_rt44.bat`.
- **Chọn giữa các bản và hướng dẫn nạp đầy đủ:** [`Firmware/README.md`](Firmware/README.md)
  (nạp bằng IDE: [`Docs/Nap_Bang_IDE.md`](Docs/Nap_Bang_IDE.md)).

## 3. Cấu trúc repo

| Thư mục | Nội dung |
|---|---|
| `Xe_Can_Bang/` | Project STM32CubeIDE. Luật điều khiển và giám sát ở `Core/Inc/balance_*.h`, FSM và vòng 200 Hz ở `Core/Src/robot_fsm.c`. |
| `ESP32_S3_Gateway/` | Sketch Arduino + trang web (`dashboard_html.h`), giao thức UART, phiên điều khiển, log RAM/Flash. |
| `Simulation/` | Mô hình Scilab, harness native dùng chung header firmware, bộ 36 ca kiểm tra, cấu hình plant. |
| `tools/` | 42 host test (`test_*.cjs`), script mô phỏng/build, tải log cứu hộ. `tools/legacy/` giữ test của bản RT8/RT9 đã bỏ. |
| `Firmware/` | RT45 và RT44 (đóng gói, dùng quay lui); RT46 nạp bằng IDE từ source. Bản cũ hơn nằm trong git history. |
| `Hardware/` | Schematic, PCB KiCad, thư viện footprint, tài liệu lắp ráp. |
| `Docs/` | Tài liệu thiết kế + báo cáo các vòng RT22 → RT42 (danh sách ở mục 7). |

## 4. Chạy nhanh

```powershell
# Kiểm thử trên máy (cần Node.js; test biên dịch C cần GCC tại C:/msys64/ucrt64/bin)
Get-ChildItem tools/test_*.cjs | ForEach-Object { node $_.FullName }

# Mô phỏng native: 12 ca tốc độ / 12 ca biên / 12 ca khởi hành 50 cm/s, mỗi ca 20 s
node tools/run_governor_sim.cjs --stress
node tools/run_governor_sim.cjs --stress --edge
node tools/run_governor_sim.cjs --stress --launch --launch50
```

- **STM32:** mở `Xe_Can_Bang/` bằng STM32CubeIDE → Build → Run (ST-LINK), hoặc nạp thẳng `.hex`.
- **ESP32:** mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` bằng Arduino IDE (board ESP32S3 Dev Module,
  Flash 16 MB, partition 3 MB APP/9.9 MB FATFS, PSRAM OPI, USB CDC on boot) → Upload.
- **Scilab:** chạy `Simulation/build_native.ps1` rồi Execute `robot_can_bang_pid_1.sce` ở gốc repo.

## 5. Đã kiểm chứng

| Hạng mục | Kết quả | Điều kiện |
|---|---|---|
| Host test | **42/42 pass** | Node.js 24, GCC (msys2); mỗi test tự biên dịch phần C tương ứng |
| Mô phỏng native | **36/36 ca đủ 20/20 s** khi chọn mức ≤ 80 (12 stress80 + 12 edge + 12 launch); **35/36** khi chạy MAX 90 | Ca còn lại là pin 11,2 V — đúng vùng bị cấm với mức 90; chạy lại sau RT39, số liệu trùng RT38 |
| Log xe thật | RT36: 154 s. RT38: 84 s. RT39: **8 log, 1 100 s** (3 lần ngã ở 90, 1 lần cắt motor khi bị giữ) → cơ sở của RT40. RT40 **chưa có log**; RT41 có log 50 s (đường lệnh ổn định, không lần nào >150 ms khi đang có lệnh); RT42 có log 148 s tại nhà (đường lệnh đạt p95 27 ms; 2 lần ngã ở MAX 90 → RT43 sửa); RT43 chưa có log | Pack 11,7 V; xem `Docs/RT36_Sua_4_Loi.md`, `Docs/RT37_Xuat_Phat_Muot.md` |
| Nguồn ↔ binary | 13/13 tệp khớp `SOURCE_SHA256.json` | Hash tính lại tại máy này, 18/11/2026 |

## 6. Giới hạn đã biết

- **Trần tốc độ thực ~0,96 m/s** (đo tại 11,7 V). Mức 0,90 chỉ dùng khi pin **≥ 12,0 V**; dưới
  ~11,9 V governor sẽ ghìm. Muốn nhanh hơn phải lên **4S** hoặc đổi motor/giảm tốc.
- **Cua ở MAX 90 tự hạ về ~0,75 m/s** (trần cua RT36) — cố ý, để bánh ngoài còn dư và hai chiều
  cua đối xứng; chạy thẳng vẫn 0,90 m/s.
- Bù lệch bánh trái (110 PWM) cân mô-men nhưng không làm bánh trái khoẻ thêm; cua trái vẫn ngả
  nhiều hơn cua phải (phần cơ khí).
- Buông ga/đảo chiều đột ngột ở 0,85–0,90 m/s có thể làm xe ngã — cần giảm tốc trước khi đảo chiều.
- Firmware không có watchdog độc lập: nếu MCU treo, TIM1 giữ duty cuối.
- Mô hình mô phỏng chưa có trượt bánh, tụt áp theo tải và tư thế trung tính +1,5°.

## 7. Tài liệu

- [`HANDOFF.md`](HANDOFF.md) — trạng thái, quy tắc vận hành, việc còn mở.
- [`Docs/Tong_Quan_Do_An.md`](Docs/Tong_Quan_Do_An.md) — tổng quan đồ án.
- [`Docs/Tinh_Nang_Xe_Hien_Hanh.md`](Docs/Tinh_Nang_Xe_Hien_Hanh.md) — chức năng đang chạy, kèm số đo.
- [`Docs/Thuat_Toan_Dieu_Khien.md`](Docs/Thuat_Toan_Dieu_Khien.md) — luật điều khiển và các lớp giám sát.
- [`Docs/Thong_So_Xe_Hien_Tai.md`](Docs/Thong_So_Xe_Hien_Tai.md) — thông số cơ khí, điện, encoder, đường đặc tính bánh.
- [`Docs/Bang_Ma_Loi_Tu_Dong_Dung.md`](Docs/Bang_Ma_Loi_Tu_Dong_Dung.md) — mọi trường hợp firmware tự cắt motor.
- [`Docs/Phan_Cung_Va_Ket_Noi.md`](Docs/Phan_Cung_Va_Ket_Noi.md), [`Huong_Dan_Di_Day_Toan_Bo_He_Thong.md`](Huong_Dan_Di_Day_Toan_Bo_He_Thong.md) — phần cứng và đấu dây.
- [`Docs/Roadmap_Do_An.md`](Docs/Roadmap_Do_An.md), [`Docs/Phat_Trien_Tiep.md`](Docs/Phat_Trien_Tiep.md) — phương pháp và hướng tiếp.
- Báo cáo từng vòng: [`RT22`](Docs/RT22_Doi_Xung_Va_Be_Huong.md) · [`RT23`](Docs/RT23_Nut_That_Va_Loi_Toc_Do.md) ·
  [`RT24`](Docs/RT24_Taper_Nghieng_Quay_Trai.md) · [`RT27`](Docs/RT27_Tro_Ve_Nen_RT21_Chong_Chiu.md) ·
  [`RT28`](Docs/RT28_Giu_Moc_Vi_Tri_Khi_Bi_Day.md) · [`RT29`](Docs/RT29_Tu_Ngat_4096.md) ·
  [`RT32`](Docs/RT32_Bo_Toc_Do_50_80_90.md) · [`RT33`](Docs/RT33_On_Dinh_Ket_Noi.md) ·
  [`RT34`](Docs/RT34_Cua_Muot.md) · [`RT35`](Docs/RT35_Mo_Phong_Khop_Log.md) ·
  [`RT36`](Docs/RT36_Sua_4_Loi.md) · [`RT37`](Docs/RT37_Xuat_Phat_Muot.md) · [`RT38`](Docs/RT38_Chuyen_Huong_Lien_Tuc.md) · [`RT39`](Docs/RT39_Cua_80_Khong_Khung.md) · [`RT40`](Docs/RT40_Het_Ngap_Ngung.md) · [`RT41`](Docs/RT41_Duong_Truyen.md) · [`RT42`](Docs/RT42_Song_2G4.md)
- [`Simulation/README.md`](Simulation/README.md), [`Hardware/README.md`](Hardware/README.md) — hướng dẫn riêng từng phần.

**Lịch sử phát triển** (RT7 → RT21, các bản nạp cũ, log thô và phân tích từng vòng) nằm trong **git
history** của repo, không giữ trong cây làm việc để repo gọn cho báo cáo.
