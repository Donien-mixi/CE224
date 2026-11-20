# Bản chốt RT41 — đường truyền nhanh hơn (UART 921600 + chọn kênh Wi-Fi)

Bản này **chỉ đổi đường truyền**: luật điều khiển, khung telemetry 122 B, vòng 200 Hz và mọi tệp
`balance_*.h` **giữ nguyên như RT40** — chỉ `balance_profile.h` đổi build ID.

Nạp **cả hai board** (UART đổi tốc độ ở cả hai đầu).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261120-R12-8DIR-RT41-LINK` |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT41-LINK-NET41` |

## 1. Vì sao chỉ hai việc trong bảy đề xuất

Đo lại đường lệnh trên **6 log RT39 có lái** (25 481 mẫu có lệnh): `age` p50 **13 ms**, p95 **28 ms**,
max 64 ms; khoảng cách hai gói lệnh khi đang giữ p50 **23 ms**, p95 **35 ms**; chỉ **0,1–0,2 %** lần
vượt 60 ms. Đường lệnh đã sát, nên chỉ hai việc có lợi ích đo được:

| # | Việc | Trước | Sau |
|---|---|---|---|
| 1 | **UART 115200 → 921600** (STM32 + ESP32 + `.ioc`) | khung 122 B giữ TX DMA **10,6/20 ms = 53 %**; lệnh 35 B mất **3,04 ms** | **1,3 ms = 6,7 %**; lệnh **0,38 ms** |
| 2 | **Chọn kênh AP ít nhiễu** trong 1/6/11 (quét một lần, ≤ ~0,8 s) | cố định kênh **6** | kênh ít AP nhất, có log `[NET41] … -> AP chN` và `/network-status` báo `channel` |

Lợi ích kèm theo của #1: `ESP32_Comm_SendBytes` **bỏ khung nếu UART đang bận**, nên ở 115200 khung
`$BINFO` (trang dùng để `matched()`) hay bị bỏ lượt; ở 921600 gần như luôn gửi được ngay.

Năm đề xuất còn lại **không làm** và có lý do đo được (xem
[`../../Docs/RT41_Duong_Truyen.md`](../../Docs/RT41_Duong_Truyen.md) mục 4): #1 đã có sẵn trong trang
(`pressDrive()` gọi `driveTick()`), #2 race `WebSocketsServer` giữa hai core, #3/#6 phải viết lại đường
dữ liệu của trang mà không kiểm được bằng trình duyệt ở đây, #7 chỉ tiết kiệm 0,2 ms sau khi đã có #4.

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Lớp điều khiển | **0 tệp `balance_*.h` bị sửa** ngoài build ID |
| Host test | **41/41 pass** |
| STM32 | 83 824 B bin; text/data/bss **giống hệt RT40** (83 320 / 496 / 7 520); 0 lỗi 0 cảnh báo |
| ESP32 | 1 091 760 B; 0 lỗi |
| IDE | build lại trong `D:\CubeIDE_ws` → hex **trùng SHA256** (`4b1efacf…`) |
| Hash | 7/7 binary, 15/15 nguồn |

## 3. Nạp

`nap_rt41.bat` (**bắt buộc** nhập COM ESP32 — nạp lệch một board là mất liên lạc) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT41_LINK_20261120'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT41_LINK.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT41_LINK_NET41.bootloader.bin" `
  0x8000 "$rel\ESP32_RT41_LINK_NET41.partitions.bin" `
  0xe000 "$rel\ESP32_RT41_LINK_NET41.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT41_LINK_NET41.app.bin"
```

Kiểm ngay sau khi nạp:

1. Serial Monitor ESP32 (115200): phải thấy `[UART] … @ 921600 bps` và
   `[NET41] scan=… ch1=… ch6=… ch11=… -> AP chN`.
2. Trang `/network-status`: `channel` = kênh trong log.
3. Một lượt chạy ngắn rồi tải log: `crc_errors=0`, `sequence_gaps=0`, `age` p95 ≤ 30 ms.

## 4. Quay lui

Nếu 921600 gây lỗi bit (`crc_errors` tăng): đổi `STM32_BAUD` trong `.ino`, `huart1.Init.BaudRate` trong
`usart.c` và `USART1.BaudRate` trong `.ioc` về `115200`, nạp lại **cả hai** board. Hoặc nạp lại
[`../RT40_HOLD_20261119/`](../RT40_HOLD_20261119/README.md).

## 5. Giới hạn

- **Chưa kiểm trên xe** — cần một lượt để xác nhận `crc_errors=0` ở 921600.
- Chọn kênh theo **số AP**, không đo nhiễu thực; nếu vẫn chậm, đặt cố định `-DGATEWAY_AP_CHANNEL=1`
  (hoặc 11).
- AP xuất hiện chậm thêm ~0,3–0,8 s do quét kênh; trang tự thử lại 20 s nên không thành “trang chết”.
