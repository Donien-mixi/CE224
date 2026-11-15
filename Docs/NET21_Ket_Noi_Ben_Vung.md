# R12 RT21-ANTISTALL / NET21 — kết nối web ↔ xe bền hơn sau cú ngã

> **Trạng thái:** đây là bản hiện hành (13/11/2026). Bản `RT21-QUICKSTART`
> (14/11, hiệu chuẩn gyro lúc boot 200 mẫu) đã được **khôi phục** về đúng cặp
> RT21-ANTISTALL / NET21 này theo yêu cầu người dùng; ghi chú thí nghiệm giữ ở
> [QUICKSTART](RT21_QUICKSTART_Hieu_Chuan_Nhanh.md).

Bản dựng 13/11/2026: STM32 `BAL-20261111-R12-8DIR-RT21-ANTISTALL` (không đổi), ESP32
`TWIP-WEB-20261113-BAL-R12-8DIR-RT21-ANTISTALL-NET21`. Artifact:
`.build/balance_release_r12_rt21_antistall_net21_20261113/` kèm `SHA256.json`,
`SOURCE_SHA256.json`. **Chưa nạp xe, chưa chạy thử trên xe.** Luật điều khiển R12,
gain/observer/phân bổ lực, encoder, REC1 và khung telemetry 122 byte không đổi; hash
STM32 trong artifact trùng đúng bản RT21 cũ.

## 1. Hiện tượng và bằng chứng đo được

Ba log RT20 (80 cm/s, tải 11/11) đều ghi trong header:

| Log | boot | reset | archive_error | samples | CRC lỗi / thiếu gói / STM32 reset |
|---|---|---|---|---|---|
| `(3) (2)` | 239 | `brownout` | 1 (`filesystem_unavailable_preserved`) | 3 344 | 0 / 0 / 0 |
| `(4) (2)` | 239 | `brownout` | 1 | 7 364 | 0 / 0 / 0 |
| `(5) (2)` | 239 | `brownout` | 1 | 5 905 | 0 / 0 / 0 |

Cùng ba log: `command_age` p50 13–14 ms, p95 34–41 ms; không có lần lease hết hạn khi
đang chạy; mọi stall ngắn đều ở lệnh trung tính. Nghĩa là **đường STM32 ↔ ESP32 không
lỗi** trong suốt lượt chạy; thứ hỏng nằm ở ESP32 và ở trang web.

Chuỗi sự kiện đọc được từ số liệu:

1. Xe ngã/đảo chiều → dòng motor tăng vọt → 5 V hoặc 3V3 tại ESP32 tụt → **brownout
   reset**. Đây là hiện tượng nguồn, không phải lỗi phần mềm.
2. Mỗi lần reset, ESP32 mất toàn bộ log RAM (log là PSRAM, 30 000 mẫu). Bản dự phòng
   duy nhất là log Flash — mà trên chính board này **`archive_error=1` ở cả ba lượt**:
   phân vùng FAT không mount được. Vì vậy sau cú ngã, dữ liệu lượt vừa chạy **mất hẳn**,
   và trang hiện "Chưa nhận STM32 đang hoạt động" cộng log rỗng → người dùng thấy
   "STM32 mất dữ liệu".
3. `archiveBegin()` cũ quét **toàn bộ** phân vùng 9,9 MB (`0x9E0000`) để kiểm tra
   "trắng" **trước khi** bật SoftAP: 20 224 lần đọc 512 B + 2 528 lần `delay(1)`
   (≈ 2,5 s chỉ riêng delay, chưa kể thời gian đọc). Cộng `delay(500)` đầu `setup()`,
   mỗi lần brownout trang phải chờ **hàng giây** trước khi có Wi-Fi để kết nối lại →
   "kết nối chậm".
4. Trang không có kiểm tra sống của WebSocket: chỉ cần socket còn `readyState=OPEN`,
   trang vẫn `send('$PING*')` mỗi 200 ms vào một kênh đã chết (Wi-Fi phone rớt không
   có FIN). `onclose` không chạy cho tới khi TCP tự timeout — trên điện thoại có thể
   vài chục giây. Người dùng thấy "mất kênh lệnh" dù Wi-Fi danh nghĩa vẫn "đã kết nối".

## 2. Đã sửa gì (chỉ ESP32 + trang web)

| # | Thay đổi | Tệp |
|---|---|---|
| 1 | SoftAP bật **trước** khi chuẩn bị log Flash; chỉ quét phân vùng khi mount thất bại | `ESP32_S3_Gateway.ino`, `balance_archive.h` |
| 2 | Volume không mount vẫn tạo mutex/buffer → mở đường phục hồi; thêm `POST /archive-format` (chỉ khi motor dừng, cần `confirm=ERASE` + phiên hợp lệ) và nút **SỬA VÙNG LOG FLASH** trên trang | `balance_archive.h`, `balance_web.h`, `dashboard_html.h` |
| 3 | Liệt kê log Flash bằng **tên tệp trần**; trước đây `f.name()` trả cả `/twip-net1/...` nên `archiveEntryIndex` từ chối (có `/`) → tải log Flash sẽ 404 dù volume mount được. Quét chỉ đọc header khối, không đọc payload | `balance_archive.h` |
| 4 | Trang đóng WebSocket sau **3 s** không nhận khung nào từ gateway, thử lại 0,5 → 1 → 2 → 4 s (chặn trần 4 s), hello reset về 0,5 s; gateway phát `{"hb":1}` 1 Hz và bật ping 20 s/8 s | `dashboard_html.h`, `ESP32_S3_Gateway.ino` |
| 5 | `POST /operate`… `/balance-status` trả thêm `boot_id`, `uptime_ms`, `reset_reason`, `ws_clients`; trang báo **"ESP32 vừa khởi động lại (SỤT ÁP NGUỒN)"** trong 12 s thay vì đổ lỗi cho STM32 | `balance_web.h`, `dashboard_html.h` |
| 6 | `poll()` dùng chung một promise đang bay: hello/arm-wait/timer không còn "tưởng đã xong" khi fetch chưa về | `dashboard_html.h` |

Không đổi: khung 122 B, BINFO, `$MOVE8`/lease 350 ms, epoch/sequence, cổng 80/81,
`/operate` ARM, log 40 cột, mọi literal build của STM32.

## 3. Kiểm chứng trên máy

Hai test mới **fail trước khi sửa** và pass sau:

- `tools/test_net21_archive.cjs` — trước sửa: ca 1 fail `g_partReadCalls==0` (vẫn quét
  phân vùng khi mount được) và fail tên tệp có `/`; ca 2 fail vì không tạo được
  mutex/buffer nên không có đường phục hồi. Sau sửa: 3/3 ca pass, gồm mount nhanh,
  volume hỏng → `archiveFormatNow()` cứu được, phân vùng trắng → tự format như cũ.
- `tools/test_net21_link.cjs` — trước sửa: không có `wsAlive`. Sau sửa: 2,9 s im lặng
  chưa đóng, 3,1 s đóng; backoff 0,5/1/2/4 s và chặn trần; hello reset; khung `hb`
  được tính là sống; đổi `boot_id` báo khởi động lại và nhả nút đang giữ; quá 12 s thì
  câu "Chưa nhận STM32" quay lại.

Hồi quy: 22/22 test host đang dùng pass (20 test cũ + 2 test NET21; thêm
`test_balance_journal.cpp` và `test_uart_schedule.py` cũng pass). Sáu test
`test_drive8_*`, `test_hardware_boot`, `test_load_speed` **vẫn fail như trước phiên
này** vì chúng còn trỏ tài nguyên đã bỏ (`id="fixture"`, `twip_load_speed_R12.txt`,
`load_speed_test.h`, tên biến trong mock `Motor_SetDuty`, build ID RT9); chúng không
đọc tệp nào vừa sửa. Không sửa assertion của chúng.

ESP32 build bằng arduino-cli đúng FQBN đã dùng: 1 086 131 B app (34 %); STM32 không
build lại, hash trùng bản RT21.

## 4. Giới hạn — phải nói rõ

- **Brownout là lỗi nguồn, firmware không chữa được.** Phải đo 5 V và 3V3 **ngay tại
  chân cấp của ESP32** khi motor khởi động/đảo chiều, và thêm tụ bulk 470–1 000 µF
  sát chân 5 V. Sửa xong thì tần suất reset mới giảm; NET21 chỉ làm cú reset **ngắn và
  không mất log**, không ngăn được reset.
- Chưa đo thời gian iPhone re-associate sau reset; con số "vài giây" là suy luận từ
  thứ tự khởi động, không phải số đo.
- `archive_error=1` trên board thật cần xác nhận lại sau khi nạp NET21: nếu nút
  **SỬA VÙNG LOG FLASH** báo lỗi thì nhiều khả năng sai phân vùng khi nạp
  (`app3M_fat9M_16MB`), không phải FAT hỏng.
- Trang bị ẩn (tab background) có thể bị throttle timer → kiểm tra sống 3 s không chạy
  đúng nhịp; khi quay lại tab, handler `visibilitychange` vẫn kết nối lại.
- Toàn bộ kết luận trên là **đọc mã + test host + build**, chưa có log NET21 trên xe.

## 5. Việc kế tiếp

1. Nạp `STM32_BAL_R12_RT21_ANTISTALL.bin/.hex` và `ESP32_RT21_NET21.merged.bin`
   (hoặc nạp sketch bằng Arduino IDE), mở `http://192.168.4.1/ui/r12?v=RT21-NET21`.
2. Đo 5 V/3V3 tại ESP32 khi motor đảo chiều; ghi lại `boot_id` trước/sau một cú ngã.
3. Nếu `archive_error` khác `ready`: bấm **SỬA VÙNG LOG FLASH** khi motor đã dừng, rồi
   chạy lại một lượt ngắn và tải log Flash để xác nhận ghi được.
4. Gửi log NET21 (TXT + `CHẨN ĐOÁN KẾT NỐI`) để đối chiếu: kỳ vọng `crc=0`, `gaps=0`,
   `STM32_resets=0`, và nếu có reset thì phải thấy `archive_saved>0` của lượt trước.
