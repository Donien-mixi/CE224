# RT42 — Nhiễu sóng 2.4 GHz: đo được, chịu được, đổi kênh được

Bản chi tiết + cách nạp: [`Firmware/RT42_RF_20261120/README.md`](../Firmware/RT42_RF_20261120/README.md).
Tài liệu này ghi phần **phân tích sóng** và các con số đo.

## 1. Log RT41 (`twip_balance_R12 (39).txt`, 20/11)

Đúng build **`BAL-20261120-R12-8DIR-RT41-LINK`** / gateway **NET41**: 2 520 mẫu / 50,4 s, `drops=0`,
`dt` 4 998–5 000 µs, `exec` p95 660 µs, `fault=0` khi `state=3`, 4 cước REC1 (23,5 % thời gian — log
này chạy mốc 90 nên REC1 nhiều là bình thường).

| Đường lệnh | RT41 | RT39 (6 log, 25 481 mẫu) |
|---|---:|---:|
| `age` p50 / p95 / p99 / max | 13 / **29** / 45 / 130 ms | 13 / 28 / — / 64 ms |
| Khe giữa 2 gói lệnh p50 / p95 / p99 | 23 / **36** / 50 ms | 23 / 35 / 50 ms |
| Số lần khe > 150 ms khi **đang có lệnh** | **0** | 0 |

Diễn giải: UART 921600 không sinh lỗi (không mất mẫu, không CRC), độ trễ **p95 29 ms** — đây là mức
tốt nhất có thể với một vòng 50 Hz + Wi-Fi. Cái còn lại là **độ chịu nhiễu**: trong 50 s chỉ có **một**
lần nghen ~140 ms (t = 92,2 s, `age` 130 ms). Ở phòng đông sóng, loại này sẽ dày hơn — và đó chính là
thứ gây cảm giác “chậm nhận tín hiệu”, không phải tốc độ UART.

## 2. Xe dùng băng nào

ESP32-S3 **chỉ có radio 2.4 GHz** (2400–2483,5 MHz; không có 5 GHz). AP phát **HT20** (20 MHz), kênh
1–13. Vì vậy mọi thứ dưới đây đều nằm cùng băng và **không thể “tránh” bằng cách đổi băng**:

| Nguồn | Ảnh hưởng | Cách giảm |
|---|---|---|
| AP hàng xóm cùng/ lân cận kênh | Chiếm thời gian sóng, gây retransmit | Chọn/đổi kênh 1-6-11 (RT41 + RT42) |
| Bluetooth/BLE (tai nghe, chuột, loa) | BT nhảy tần toàn băng → không kênh nào sạch | Giảm số khung, tăng chịu nghen (lease 500 ms) |
| Camera 2.4 GHz, lò vi sóng | Băng rộng, chiếm sóng mạnh | Tránh xa nguồn; đổi kênh |
| Điện thoại tự quét Wi-Fi / kiểm tra “có internet” | Mỗi lần quét AP treo 100–300 ms | Giữ điện thoại gần; tắt tự chuyển mạng; (bước sau: captive-portal) |
| Nhiễu bo mạch (USB 3, buck, dây nguồn) | Hài 2.4 GHz | Ăng-ten xa cáp nguồn, đã hạ TX 11 dBm từ RT34 |

## 3. Đã làm trong RT42

| Việc | Chi tiết |
|---|---|
| **Đo sóng trong mọi log** | Dòng `# RADIO ch=… rssi=… phy=…` + `ws_joins/leaves`, `ap_joins/leaves/reason`. `rssi` = dBm của điện thoại đang điều khiển; `phy` = 11b/11g/11n. |
| **Đổi kênh khi đang đỗ** | `POST /network-channel` → quét 1/6/11, gọi `esp_wifi_set_channel`, trả `{before,after,changed,scan,ch1,ch6,ch11}`. Từ chối khi xe đang cân bằng hoặc đang tải log. |
| **Chịu nghen ngắn** | Lease lệnh `BALANCE_MOTION_STALE_MS` 350 → **500 ms**: một cú nghen 300–400 ms không còn làm lệnh bị coi là mất. |
| **Giảm tải sóng** | HTTP poll 100 → **200 ms** (mỗi request là một vòng TCP + JSON). |

## 4. Cách đọc kết quả trong log mới

| `rssi` | `phy` | Kết luận | Việc nên làm |
|---|---|---|---|
| −35…−55 | 11n | Tốt | Nếu vẫn chậm ⇒ nhiễu ⇒ đổi kênh |
| −56…−70 | 11n | Trung bình | Gần hơn, bỏ vật cản |
| < −70 | 11b/11g | Yếu/nhiễu nặng | Đổi kênh, gần hơn, tắt BT |
| tốt | 11b | Rớt tốc độ do nhiễu | Đổi kênh; kiểm nguồn nhiễu gần ăng-ten |

## 5. Việc còn mở (theo thứ tự)

1. **Captive-portal + DNS responder**: trả lời `generate_204` / `hotspot-detect.html` / `connecttest.txt`
   và một DNS trả `192.168.4.1`, để điện thoại thôi kiểm tra “có internet” và thôi quét lại. Cần thêm
   một tác vụ UDP; chưa làm vì không kiểm được bằng trình duyệt ở đây.
2. **Đo chiếm sóng thật** (không chỉ số AP): cần máy phân tích phổ hoặc log `esp_wifi` thống kê
   retransmit; nếu có, đưa vào `/network-status` để quyết định kênh bằng số liệu.
3. **Giảm payload `/balance-status`**: vẫn cần viết lại đường dữ liệu của trang (như đã ghi ở RT41 mục 4).
