# RT33 — Vì sao xe hay mất kết nối (ESP32 tự khởi động lại) và cách sửa

Ngày 17/11/2026. Phần này **chỉ sửa gateway ESP32** (NET34); firmware STM32 giữ nguyên RT32-508090,
nên chỉ cần nạp lại ESP32.

## 1. Hiện tượng

Trong lúc chạy, trang web mất kết nối rồi hiện banner:

> **ESP32 ĐÃ KHỞI ĐỘNG LẠI (watchdog ngắt).** Không tự bật motor. Đang chờ STM32 gửi lại dữ liệu;
> log Flash còn **0 mẫu**.

Kèm theo, các log gần đây có `archive_saved=0`, `archive_error=1`.

## 2. Kết luận: **ESP32 reset, không phải STM32**

Bằng chứng thu được:

1. Banner do chính gateway in ra khi nó phát hiện `boot_id` đổi — và nó in **lý do reset** lấy từ
   `esp_reset_reason()`. Ảnh của bạn ghi `watchdog ngắt` = **`ESP_RST_INT_WDT`** (watchdog ngắt,
   tức CPU bị khoá trong vùng tắt ngắt quá lâu).
2. Trong khi đó, ngay sau reset, trang vẫn hiện **“ĐANG CHẠY CÂN BẰNG R12”** với góc/tốc độ/PWM thật
   → **STM32 vẫn sống và vẫn cân bằng**. Chỉ board Wi-Fi khởi động lại.
3. Thống kê **toàn bộ 57 log** trong `Downloads` (đọc dòng `# GATEWAY_NET1 … reset=`):

| `reset=` | số log |
|---|---:|
| `power_on` | 48 |
| **`brownout`** | **8** |
| **`interrupt_watchdog`** | **1** |

Ba log RT32 mới nhất `(3)(5)/(4)(5)/(5)(5)` đều **`reset=brownout`**, `boot=457`, `archive_error=1`.

## 3. Chuỗi nguyên nhân (khớp với cả hai loại reset)

```
motor đảo chiều / bứt tốc  →  dòng tăng vọt  →  5 V sụt  →  ESP32 brownout  →  reset
        │                                                          │
        │                                                          └─ nếu đang ghi Flash: FAT hỏng
        │                                                             → archive_error=1 (mất log Flash)
        └─ muốn sửa archive phải format 9,9 MB (f_mkfs, cache bị tắt hàng chục giây)
              → watchdog ngắt (ESP_RST_INT_WDT)  →  reset thêm một lần
```

Chi tiết kỹ thuật:

- **Brownout**: ESP32-S3 ngắt khi 3V3 tụt dưới ngưỡng (~2,7 V). Nguồn 5 V đang dùng chung với driver
  motor A4950, nên mỗi cú đảo chiều tạo xung dòng lớn. Đỉnh dòng WiFi đóng góp một phần.
- **Interrupt watchdog**: `archiveFormatNow()` gọi `FFat.begin(true,…)` = `f_mkfs` trên phân vùng
  9,9 MB. Trong lúc xoá/format, cache Flash bị tắt và ngắt bị chặn từng đợt dài ⇒ watchdog ngắt
  (ngưỡng 300 ms) bắn. Đây là lý do “bấm SỬA VÙNG LOG FLASH xong ESP32 khởi động lại”.
- **FAT hỏng**: nếu brownout rơi đúng lúc đang ghi/ghi-một-phần sector ⇒ volume không mount lại được
  ⇒ từ đó mọi lần boot đều `archive_error=1`, log chỉ nằm RAM.

## 4. Đã sửa trong NET34 (chỉ ESP32)

| Thay đổi | Tác dụng |
|---|---|
| `WiFi.setTxPower(WIFI_POWER_11dBm)` (trước 17 dBm) | giảm đỉnh dòng WiFi ~100 mA ⇒ bớt sụt áp khi motor đảo chiều. Tầm phát vẫn đủ trong phòng/sân nhỏ |
| **Không ghi Flash khi xe đang cân bằng/chạy**: archive task chỉ ghi khi đã `settled` (state ≠ 3/7 và hết đuôi sau REC1) | bỏ hoàn toàn cửa sổ “cache tắt + FAT đang ghi” trong lúc chạy ⇒ không còn hỏng FAT do brownout giữa lượt; log vẫn đủ vì RAM giữ ~6 phút và trang chỉ cho tải log sau khi STOP |
| Build ID gateway `…-NET34` | phân biệt bản; phần `BAL-…-RT32-508090` giữ nguyên nên vẫn khớp STM32, không cần nạp lại STM32 |

Không đổi: khung 122 B, lease, STOP, log 40 cột, trang web, giao thức UART.

## 5. Việc phải làm ở phần cứng (quan trọng hơn firmware)

1. **Tụ bù tại ESP32**: 1000 µF low-ESR trên 5 V (ngay chân board) + 100 µF + 10 µF trên 3V3.
2. **Nguồn riêng cho ESP32**: lấy từ XL4015/buck riêng hoặc nhánh riêng trước node driver motor;
   dây ngắn, tiết diện lớn; mass hình sao tại cực âm pack.
3. **Đo bằng oscilloscope** 5 V và 3V3 tại ESP32 đúng lúc đảo chiều motor — nếu 5 V tụt < 4,5 V thì
   phải xử lý nguồn, firmware không cứu được.

## 6. Quy trình vận hành sau khi nạp NET34

1. Nạp **chỉ ESP32** (STM32 giữ RT32).
2. Kiểm banner **`…-NET34`**; log header phải ghi `reset=power_on` (nếu vẫn `brownout` → nguồn chưa đủ).
3. **Không bấm “SỬA VÙNG LOG FLASH” khi xe đang bật cân bằng.** Nếu cần format: tắt motor, để ESP32
   chạy một mình, bấm format và **chờ ~30 s** (có thể ESP32 tự khởi động lại một lần — bình thường).
4. Sau mỗi lượt: STOP → chờ 2 s → tải log (lúc này archive mới ghi).

## 7. Việc còn mở (cần bạn quyết)

- **STM32 heartbeat 10 s**: nếu ESP32 khởi động lại giữa lượt, sau 10 s không nhận lệnh STM32 sẽ
  `BalanceStop(2)` và xe ngã. Có thể nâng lên 15 s để gateway kịp khởi động lại, nhưng như vậy giảm
  mức bảo vệ “mất liên lạc thì dừng”. Đây là đánh đổi an toàn — cần bạn chọn trước khi mình sửa.
- Nếu sau khi thêm tụ mà vẫn còn `brownout`: kiểm tra pack 3S khi tải nặng (dưới 10,5 V là pack yếu),
  và xem lại XL4015 (dòng tối đa 5 A nhưng sụt áp khi tải xung).
