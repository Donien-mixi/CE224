# RT41 — Đường truyền: UART 921600 + chọn kênh Wi-Fi

Bản này **chỉ đổi đường truyền**. Luật điều khiển, khung telemetry 122 B, vòng 200 Hz và toàn bộ
`balance_*.h` **giữ nguyên như RT40** (`git diff` của bản này chỉ có `usart.c`, `.ioc`, các tệp ESP32 và
build ID).

> **Phải nạp cả hai board.** UART đổi tốc độ ở cả hai đầu; nạp lệch là mất liên lạc (xe vẫn cân bằng
> nhưng trang không điều khiển được, và sau 10 s không có lệnh thì motor dừng).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261120-R12-8DIR-RT41-LINK` |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT41-LINK-NET41` |

## 1. Vì sao chỉ làm hai việc này

Người vận hành đề xuất 7 cải tiến đường truyền. Trước khi sửa, tôi đo lại đường lệnh trên **6 log
RT39** (có lái, 25 481 mẫu có lệnh):

| Chỉ số | Đo được |
|---|---|
| `age` (mẫu log − lệnh cuối STM32 nhận) | p50 **13 ms**, p95 **28 ms**, max **64 ms** |
| Khoảng cách giữa hai gói lệnh khi đang giữ | p50 **23 ms**, p95 **35 ms**, p99 50 ms |
| Số lần khoảng cách > 60 ms | **0,1–0,2 %** |

Tức là đường lệnh **đã rất sát**: lệnh tới STM32 trong ~23 ms, p95 35 ms. Không có “khựng 30–200 ms”
như bảng dự kiến — các lần khựng 1–2 s mà người vận hành thấy là REC1 trong luật điều khiển, đã sửa ở
RT40. Vì vậy chỉ hai việc dưới đây có lợi ích đo được, phần còn lại ghi rõ lý do không làm (mục 4).

## 2. Đã làm

| # | Việc | Ở đâu | Trước | Sau |
|---|---|---|---|---|
| 1 | **UART 115200 → 921600** (cả hai đầu, có `#ifndef` để quay lại) | `usart.c` + `Xe_Can_Bang.ioc` (`USART1.BaudRate`) + `STM32_BAUD` | khung telemetry 122 B giữ TX DMA **10,6 ms / mỗi 20 ms = 53 %**; lệnh 35 B cần **3,04 ms** đường truyền | **1,3 ms = 6,7 %**; lệnh **0,38 ms** |
| 2 | **Chọn kênh AP ít nhiễu** trong 1/6/11 (quét một lần, có giới hạn thời gian) | `ESP32_S3_Gateway.ino` | cố định kênh **6** | kênh ít AP nhất; log Serial `[NET41] scan=… ch1=… ch6=… ch11=… -> AP chN`; `/network-status` báo `channel` thật |

Chi tiết #1: TX dùng `HAL_UART_Transmit_DMA` và `ESP32_Comm_SendBytes` **bỏ khung nếu kênh đang bận**.
Ở 115200 kênh bận 53 % thời gian nên khung `$BINFO` (gửi 1 Hz, dùng cho `matched()` của trang) hay bị
bỏ lượt; ở 921600 chỉ còn 6,7 % nên gần như luôn gửi được ngay. Đây là lợi ích thứ hai, ngoài 2,6 ms
của bản thân lệnh.

Chi tiết #2: quét có giới hạn `GATEWAY_AP_SCAN_MS_PER_CHANNEL 60` (13 kênh ⇒ tối đa ~0,8 s, đo được
0,3–0,8 s) **trước khi** bật AP, đúng thứ tự cũ. Không quét được thì giữ kênh 6. Đặt
`-DGATEWAY_AP_CHANNEL_SCAN=0` để tắt hoàn toàn.

## 3. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| `git diff` lớp điều khiển | **0 tệp `balance_*.h` bị sửa** — chỉ `balance_profile.h` đổi build ID |
| Host test | **41/41 pass** (thêm `gatewayApChannel` vào mock của `test_net1_routes`) |
| Build STM32 | 83 824 B bin, text/data/bss **giống hệt RT40** (83 320 / 496 / 7 520), 0 lỗi 0 cảnh báo |
| Build ESP32 | 1 091 760 B (RT40: 1 091 072 B), 0 lỗi |
| IDE | build lại trong `D:\CubeIDE_ws` ra hex **trùng SHA256** bản đóng gói (`4b1efacf…`) |
| Hash | 7/7 binary + 15/15 tệp nguồn |

## 4. Bảy đề xuất — làm gì và vì sao

| # | Đề xuất | Quyết định | Lý do (đo được) |
|---|---|---|---|
| 1 | Gửi lệnh ngay khi bấm/nhả | **Đã có từ trước** | `pressDrive()` kết thúc bằng `driveTick();` và `releaseDrive()` gọi `driveTick(true)` — không còn 20 ms nào để cắt |
| 2 | Tách `webSocket.loop()` sang core 0 | **Không làm** | `WebSocketsServer` **không thread-safe**: gọi từ hai core trong khi `balance_web.h` cũng gọi `webSocket.connectedClients()`/`broadcastTXT()` là race → nguy cơ treo gateway giữa lúc xe đang chạy. Lợi ích thật ≤ 15–30 ms ở đuôi p99, không phải 30–200 ms (đo: p99 khoảng cách lệnh 50 ms) |
| 3 | Bỏ/thưa HTTP poll, đẩy status qua WS 10 Hz | **Không làm (bản này)** | Muốn thưa poll mà không mất độ tươi thì phải viết lại đường dữ liệu của trang (thêm khung WS + đổi `data.row`/`data.meta`), mà **không thể kiểm bằng trình duyệt ở đây**. Poll 100 ms hiện chỉ chiếm ~3 % CPU gateway và không làm chậm lệnh (p95 35 ms). Ghi vào việc còn mở |
| 4 | UART 921600 | **Đã làm** | Xem mục 2 |
| 5 | Kênh Wi-Fi 1/6/11 | **Đã làm** | Xem mục 2 |
| 6 | Giảm payload `/balance-status` | **Không làm** | Trang dùng `data.meta` cho `armAngleLimit()` (m[19]), `matched()` (m[17]), bias gyro (m[12..14]), `wheelAssist()`, và `row[...]` tới chỉ số 38. Bỏ cột là phải sửa đúng các chỗ đó — cùng lý do #3. Bù lại: payload ~620 B × 10 Hz ≈ 7,8 kB/s, dưới 1 % băng thông Wi-Fi nên không phải nút cổ chai |
| 7 | Lệnh nhị phân 8 B thay ASCII 35 B | **Không làm** | Sau #4, ASCII chỉ còn **0,38 ms**; bản nhị phân tiết kiệm 0,2 ms — không đo được trên xe, nhưng phải sửa cả parser STM32 và gateway (rủi ro cao hơn lợi ích) |

## 5. Cách kiểm sau khi nạp

1. Mở Serial Monitor của ESP32 (115200) — dòng đầu phải có
   `[NET41] scan=<n> ch1=… ch6=… ch11=… -> AP ch<kênh>` và `[UART] … @ 921600 bps`.
2. Vào trang, mở `/network-status` (hoặc nút mạng) — `channel` phải bằng kênh trong dòng log.
3. Chạy 1 lượt ngắn rồi tải log: `crc_errors=0`, `sequence_gaps=0`, `age` p95 phải **≤ 30 ms** như cũ
   (nếu UART 921600 bị lỗi bit thì `crc_errors` tăng ngay — đó là dấu hiệu phải quay lại 115200).
4. Nếu nghi ngờ đường UART: đổi `STM32_BAUD` trong `.ino` và `huart1.Init.BaudRate` trong `usart.c`
   (và `USART1.BaudRate` trong `.ioc`) về `115200` rồi nạp lại **cả hai** board.

## 6. Giới hạn

- **Chưa kiểm trên xe.** Đây là thay đổi đường truyền; luật điều khiển y hệt RT40 nhưng vẫn cần một
  lượt chạy để xác nhận `crc_errors=0` ở 921600 và kênh Wi-Fi mới.
- Chọn kênh chỉ dựa trên **số AP** (không đo nhiễu thực). Nếu khu vực đông và kênh chọn vẫn chậm, đặt
  `-DGATEWAY_AP_CHANNEL=11` (hoặc 1) cố định để thử.
- Quét kênh làm AP xuất hiện chậm thêm ~0,3–0,8 s; trang vẫn tự thử lại 20 s nên không thành “trang chết”.

## 7. Cập nhật 21/11 (sửa trực tiếp, GIỮ build ID RT47)

Theo yêu cầu không tạo bản firmware mới: chỉ sửa ESP32 (trang + gateway), **không đổi STM32**, không bump ID.

| Việc | Trước | Sau |
|---|---|---|
| Telemetry lên trang | HTTP poll `/balance-status` 200 ms (~620 B/lần, 5 Hz) | **WS push 20 Hz** hàng CSV (`{"t":"r",...}`) + trạng thái đầy đủ 1 Hz (`{"t":"s",...}`) |
| Poll HTTP | 200 ms | **2 s** (chỉ còn là dự phòng, cập nhật meta/chủ kênh) |
| Tick lệnh | 20 ms | **10 ms** (`setInterval(driveTick,10)`) |
| STM32 | — | không đổi (không cần nạp lại) |

Ảnh hưởng: trang tươi 20 Hz thay vì 5 Hz; bớt 5 request HTTP/s; lệnh gửi dày hơn gấp đôi (lease 500 ms vẫn giữ).
Bằng chứng đã kiểm: ESP32 build 1 094 283 B (0 lỗi); **38/38 host test** (gồm `test_net21_link` chạy chính JS của trang); `test_build_id_sync` PASS.

Cách kiểm trên xe: nạp **ESP32** (STM32 giữ nguyên), mở trang (Ctrl+F5), thấy số liệu cập nhật mượt; sau lượt chạy tải log xem
`age` p95, `crc_errors=0`, `ws_leaves=0`; `/network-status` xem `loop_gap_max_ms`.
