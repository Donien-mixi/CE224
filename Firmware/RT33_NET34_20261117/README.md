# ESP32 NET34 — sửa lỗi mất kết nối (chỉ nạp board Wi-Fi)

Ngày 17/11/2026. **Chỉ có firmware ESP32** trong thư mục này. STM32 giữ nguyên bản chốt
`BAL-20261117-R12-8DIR-RT32-508090` (trong `Firmware/RT32_508090_20261117/`) — **không cần nạp lại
STM32**, vì build ID gateway vẫn chứa đúng phần `BAL-…-RT32-508090` nên hai board vẫn khớp phiên bản.

| Board | Build ID |
|---|---|
| ESP32-S3 | `TWIP-WEB-20261117-BAL-R12-8DIR-RT32-508090-NET34` |
| STM32 (đang nạp) | `BAL-20261117-R12-8DIR-RT32-508090` |

## 1. Sửa gì trong bản này

| Thay đổi | Vì sao |
|---|---|
| TX WiFi **17 dBm → 11 dBm** | Giảm ~100 mA đỉnh dòng, bớt sụt áp 5 V khi motor đảo chiều. Đây là nguyên nhân chính gây `reset=brownout` (8 log đã ghi nhận) |
| **Không ghi Flash khi xe đang chạy/cân bằng** — chỉ ghi khi đã STOP (state ≠ 3/7 và hết đuôi sau REC1) | Bỏ cửa sổ “cache Flash bị tắt + FAT đang ghi”, nên brownout không còn làm hỏng vùng log (`archive_error=1`), và không còn khựng do ghi Flash giữa lượt. Log vẫn đủ: RAM giữ ~6 phút và trang chỉ cho tải log sau khi STOP |
| Build ID `…-NET34` | Phân biệt bản; phần `BAL-…` giữ nguyên nên STM32 không cần nạp lại |

Không đổi: khung 122 B, 40 cột, lease 350 ms, STOP, trang web, giao thức UART, toàn bộ luật điều khiển.

## 2. Nạp (chỉ ESP32, COM của board Wi-Fi)

```powershell
$rel = 'D:\DA_HTN\Firmware\RT33_NET34_20261117'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0     "$rel\ESP32_RT32_508090_NET34.bootloader.bin" `
  0x8000  "$rel\ESP32_RT32_508090_NET34.partitions.bin" `
  0xe000  "$rel\ESP32_RT32_508090_NET34.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT32_508090_NET34.app.bin"
```

Kiểm: banner **`…-NET34`**; ba nút tốc độ vẫn **50 / 80 / MAX 90**; log header dòng `GATEWAY` phải
ghi **`reset=power_on`** (nếu vẫn `brownout` thì nguồn chưa đủ — xem mục 4).

## 3. Quy trình dùng (quan trọng)

1. **Đừng bấm “SỬA VÙNG LOG FLASH” khi xe đang bật cân bằng.** Format 9,9 MB làm ESP32 khởi động
   lại (watchdog ngắt) — đó là hành vi đã biết, không phải lỗi mới.
2. Muốn format: tắt motor, để ESP32 chạy một mình, bấm format, **chờ ~30 s** (có thể tự restart 1 lần).
3. Sau mỗi lượt: **STOP → chờ ~2 s → tải log** (lúc này mới ghi Flash).
4. Nếu mất kết nối giữa lượt: **STM32 vẫn tự cân bằng**, nhưng sau 10 s không nhận lệnh nó sẽ dừng
   motor (bảo vệ mất liên lạc). Đừng cố giữ ga trong lúc mất kết nối.

## 4. Việc phần cứng phải làm (nếu vẫn còn brownout)

- 1000 µF low-ESR trên 5 V ngay chân ESP32; 100 µF + 10 µF trên 3V3.
- Cấp nguồn ESP32 từ nhánh/buck riêng, không dùng chung node với driver motor; dây ngắn, mass sao.
- Đo oscilloscope 5 V/3V3 tại ESP32 lúc đảo chiều motor; 5 V tụt dưới ~4,5 V là phải xử lý nguồn.

## 5. Đã kiểm

- **31/31 host test pass** (gồm `test_net21_archive`, `test_production_gateway/dashboard/session`,
  `test_build_id_sync`).
- ESP32 build 0 lỗi: 1 090 923 B (34 %), RAM tĩnh 54 004 B (16 %).
- `.bin` chứa `NET34` + `BAL-…-RT32-508090`, không còn `NET33`.
- **Chưa kiểm trên xe** — cần một phiên chạy để xác nhận hết `brownout` và log Flash ghi đủ sau STOP.
