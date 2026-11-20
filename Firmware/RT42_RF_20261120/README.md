# Bản chốt RT42 — chống nhiễu sóng 2.4 GHz và tự đo chất lượng sóng

Nạp **cả hai board** (lease lệnh đổi trên STM32, header log + chẩn đoán sóng đổi trên ESP32).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261120-R12-8DIR-RT42-RF` |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT42-RF-NET42` |

## 1. Log RT41 nói gì (bạn vừa gửi)

`twip_balance_R12 (39).txt` — đúng **RT41-LINK / NET41**, 2 520 mẫu / 50,4 s, `drops=0`, `dt` 4 998–5 000 µs,
`exec` p95 660 µs, không `fault` khi `state=3`.

| Chỉ số đường lệnh | RT41 (log này) | RT39 (6 log, 25 481 mẫu) |
|---|---:|---:|
| `age` p50 / p95 / p99 / max | **13 / 29 / 45 / 130 ms** | 13 / 28 / — / 64 ms |
| Khoảng cách 2 gói lệnh p50 / p95 / p99 | 23 / 36 / 50 ms | 23 / 35 / 50 ms |
| Lần > 150 ms **khi đang có lệnh** | **0** | 0 |
| REC1 entries / thời gian ở REC1 | 4 cước / 23,5 % | 28 cước / 30,2 % |

Kết luận: đường truyền **đã ổn định**, UART 921600 không gây lỗi (`drops=0`, không mất mẫu). Cái còn thiếu
không phải tốc độ mà là **khả năng chịu nhiễu**: trong 50 s này có đúng **một** lần nghen ~140 ms
(t=92,2 s, `age` 130 ms) — trong phòng nhiều sóng thì những lần như vậy sẽ dày hơn và mới là thứ làm bạn
thấy “chậm nhận tín hiệu”.

## 2. Trả lời câu hỏi: xe dùng Wi-Fi 2.4 GHz đúng không?

**Đúng, và chỉ 2.4 GHz.** ESP32-S3 chỉ có radio 2.4 GHz (2400–2483,5 MHz), không có 5 GHz. AP của xe
chạy **HT20** (20 MHz) ở kênh 1–13. Hệ quả:

| Nguồn nhiễu | Cùng băng? | Xử lý được bằng gì |
|---|---|---|
| Wi-Fi hàng xóm (AP khác) | Có, nhất là cùng kênh | **Chọn kênh** (RT41 lúc khởi động, RT42 **đổi được khi đang đỗ**) |
| Bluetooth / BLE (tai nghe, loa, điện thoại) | Có — BT nhảy tần khắp băng | Không chọn kênh nào tránh được; phải **giảm thời gian chiếm sóng** và **tăng chịu nghen** |
| Camera 2.4 GHz, lò vi sóng, chuột không dây | Có | Như trên |
| Chính điện thoại của bạn quét Wi-Fi định kỳ | Có — mỗi lần quét AP bị treo 100–300 ms | Giữ điện thoại gần, tắt “tự động chuyển mạng”, tắt BT nếu không dùng |
| Nhiễu từ chính bo mạch (USB 3, buck) | Có (hài 2.4 GHz) | Ăng-ten xa cáp nguồn; đã hạ TX 11 dBm từ RT34 |

Vì vậy RT42 làm hai việc: **đo được nhiễu** (để biết là nhiễu hay do xa) và **chịu được nghen ngắn**.

## 3. RT42 sửa gì

| # | Việc | Ở đâu | Tác dụng |
|---|---|---|---|
| 1 | **Mỗi log mang theo bằng chứng sóng**: dòng `# RADIO ch=… rssi=… phy=… ws_joins=… ws_leaves=… ap_joins=… ap_leaves=… ap_reason=…` | `balance_web.h` + `network_health.h` | Từ giờ log tự nói: kênh đang dùng, **dBm của điện thoại**, và điện thoại đang ở **11b/11g/11n**. Không cần đoán |
| 2 | **RSSI + PHY + channel** trong `/network-status` | `network_routes.h` | Trang/nút mạng thấy ngay chất lượng sóng |
| 3 | **Đổi kênh AP khi đang đỗ**: `POST /network-channel` (quét 1/6/11 rồi `esp_wifi_set_channel`) | `network_routes.h` | Khi phòng đổi môi trường sóng, đổi kênh **không cần nạp lại**; từ chối nếu xe đang cân bằng hoặc đang tải log |
| 4 | **Lease lệnh 350 → 500 ms** (`BALANCE_MOTION_STALE_MS`) | `balance_motion.h`, `balance_turn.h` | Một cú nghen 300–400 ms không còn làm lệnh bị coi là “mất” → không phanh oan giữa lúc chạy. Khi mất kết nối thật chỉ chậm 150 ms so với trước |
| 5 | **HTTP poll 100 → 200 ms** | `dashboard_html.h` | Giảm một nửa số request (mỗi request là 1 vòng TCP + JSON), nhường sóng cho kênh lệnh |

Không đổi: gain/observer/phân bổ R12, trần cua, REC1 (16°/8°/150 °/s/0,35 m/s), interlock, heartbeat 10 s,
khung 122 B.

## 4. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **41/41** (sửa `test_net1_routes` mock cho RSSI/kênh, `test_rt11_response` dùng `BALANCE_MOTION_STALE_MS`) |
| STM32 | bin 83 824 B; text/data/bss **giống hệt RT41** (83 320 / 496 / 7 520) |
| ESP32 | 1 094 288 B (RT41: 1 091 760) |
| IDE | build headless trong workspace sạch `D:\CubeIDE_ws_verify` → hex **trùng SHA256** (`d65d152c…`) |
| Hash | 7/7 binary, 16/16 nguồn |

## 5. Cách đọc “nhiễu hay do xa”

Sau khi nạp, mở log bất kỳ và xem dòng `# RADIO`:

| `rssi` | `phy` | Kết luận | Nên làm |
|---|---|---|---|
| −35…−55 | `11n` | Sóng tốt | Nếu vẫn chậm: do nhiễu → bấm đổi kênh |
| −56…−70 | `11n` | Trung bình | Đưa điện thoại gần xe hơn, tránh vật cản |
| −70…−85 | `11b`/`11g` | **Yếu/nhiễu**: đã rớt tốc độ | Đổi kênh, giữ điện thoại trong tầm 2–3 m, tắt BT |
| — | `11b` mà rssi tốt | Có nhiễu mạnh hoặc AP bị “dính” tốc độ thấp | Đổi kênh; nếu vẫn vậy, xem lại nguồn nhiễu gần ăng-ten |

Cách đổi kênh khi đang đỗ (xe **không** cân bằng):

```powershell
Invoke-RestMethod -Method Post http://192.168.4.1/network-channel
# {"before":6,"after":1,"changed":true,"scan":7,"ch1":2,"ch6":5,"ch11":3}
```

Điện thoại sẽ tự kết nối lại sau ~1–2 s.

## 6. Nạp

`nap_rt42.bat` (**bắt buộc** nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT42_RF_20261120'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT42_RF.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT42_RF_NET42.bootloader.bin" `
  0x8000 "$rel\ESP32_RT42_RF_NET42.partitions.bin" `
  0xe000 "$rel\ESP32_RT42_RF_NET42.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT42_RF_NET42.app.bin"
```

## 7. Giới hạn

- **Chưa kiểm trên xe.** Đây là thay đổi lớp truyền thông + một hằng số thời gian của lease; cần một log
  mới để thấy dòng `# RADIO` và xác nhận `crc_errors=0`, `drops=0`.
- Chọn kênh theo **số AP** (và kênh lân cận), không đo mức chiếm sóng thực. Nếu log cho thấy `rssi` tốt
  mà vẫn chậm, phải đo thêm bằng máy phân tích phổ hoặc thử cố định từng kênh.
- Chưa làm “cổng captive-portal” (trả lời `generate_204`/`hotspot-detect` + DNS) để điện thoại thôi quét
  lại: cần thêm một tác vụ UDP DNS, và không thể kiểm bằng trình duyệt trong môi trường này.
