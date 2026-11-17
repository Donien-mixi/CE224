# Bản chốt RT32 — bộ tốc độ 50 / 80 / 90 cm/s + bù lệch bánh trái · 17/11/2026

Đây là **firmware chính thức** trên nhánh `main` (thay RT29-NOCUT). Nó kế thừa toàn bộ chức năng
của RT29 và thêm hai thay đổi đã kiểm chứng qua bản thử:

| Thay đổi | Nội dung |
|---|---|
| Bộ tốc độ | **50 / 80 / 90 cm/s** (trước: 50 / 72 / 80). Preset 1 = 80, preset 2 (MAX) = 90 |
| Bù lệch bánh trái | Cộng 110 PWM cho bánh trái theo dấu quay, kẹp ±2499, đặt sau bộ phân bổ R12 |

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261117-R12-8DIR-RT32-508090` |
| ESP32-S3 | `TWIP-WEB-20261117-BAL-R12-8DIR-RT32-508090-NET33` |

## 1. Tệp trong thư mục

| Tệp | Dùng để |
|---|---|
| `STM32_BAL_R12_RT32_508090.hex` | Nạp STM32 qua ST-LINK / STM32CubeProgrammer (khuyến nghị) |
| `STM32_BAL_R12_RT32_508090.bin` | Nạp thô tại `0x08000000` |
| `STM32_BAL_R12_RT32_508090.elf` | Ký hiệu debug |
| `ESP32_RT32_508090_NET33.bootloader.bin` | esptool `0x0` |
| `ESP32_RT32_508090_NET33.partitions.bin` | esptool `0x8000` |
| `ESP32_RT32_508090_NET33.boot_app0.bin` | esptool `0xe000` |
| `ESP32_RT32_508090_NET33.app.bin` | esptool `0x10000` |
| `SHA256.json` / `SOURCE_SHA256.json` | Hash binary / hash 13 tệp nguồn |

## 2. Nạp

**Nhanh nhất:** nháy đúp `nap_rt32.bat`, nhập số COM của ESP32 (hoặc bỏ trống nếu chỉ nạp STM32).

Hoặc chạy tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT32_508090_20261117'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'

& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT32_508090.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0     "$rel\ESP32_RT32_508090_NET33.bootloader.bin" `
  0x8000  "$rel\ESP32_RT32_508090_NET33.partitions.bin" `
  0xe000  "$rel\ESP32_RT32_508090_NET33.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT32_508090_NET33.app.bin"
```

Nạp **cả hai board**. Kiểm: banner **`RT32-508090-NET33`**, ba nút tốc độ ghi **50 / 80 / MAX 90**.

## 3. Vận hành theo điện áp pin (firmware không đo được pin)

Đo pin bằng VOM trước mỗi phiên. Dải 3S của xe: **12,2 V (đầy) – 11,2 V (cạn)**.

| Pin đo được | Dùng mức nào |
|---|---|
| ≥ 12,0 V | 50 / 80 / **MAX 90** đều được; 90 là mức dùng cho demo |
| 11,7 – 12,0 V | 50 / 80; **90 chỉ nên thử tiến, chưa quay/chéo** |
| < 11,7 V | **50 / 80** — 90 sẽ bị governor ghìm (xe ì) |
| < 11,4 V | như trên, và chiều lùi ở 80 cũng sát ngưỡng |

Duty cần khi chạy đều (đo từ log thật): 80 cm/s cần **0,790 @12,2 V · 0,823 @11,7 V · 0,859 @11,2 V**;
90 cần **0,879 · 0,900 · 0,955**. Ngưỡng governor coi là hết dự trữ: **0,90**.

## 4. Đã kiểm trên máy

| Hạng mục | Kết quả |
|---|---|
| Host test | **31/31 pass** (`test_rt32_set` kiểm 3 mốc + bù bánh trái; probe 0,72/0,80/không bù **phải fail**) |
| Mô phỏng `--stress` ở **MAX 80** | **12/12** ca đủ 20 s |
| Mô phỏng `--stress` ở **MAX 90** | **9/12** — ngã 3 ca: hai ca **điện áp 11,2 V** và ca **motor yếu 20 %** |
| Mô phỏng `--stress --edge` | 12/12 |
| Mô phỏng `--stress --launch --launch50` | 12/12 |
| Build ID trong bin | đúng RT32/NET33, không còn literal RT29 |

Ba ca ngã khi MAX 90 đúng là **vùng vận hành bị cấm** ở bảng mục 3 (pin cạn / motor yếu), không phải
hồi quy. Khi chọn đúng mức theo điện áp, mô phỏng đạt 36/36.

## 5. Quay về bản cũ

`Firmware/RT29_NOCUT_20261115/` vẫn còn trong repo (cùng thư mục `Firmware/`). Nạp lại **cả hai board**.

## 6. Nguồn

Nhánh `main`, commit ghi "RT32: bo toc do 50/80/90 va bu lech banh trai".
Bản thử trước đó (không dùng cho vận hành): nhánh `test/speed-086` (0,86) và `test/speed-090` (0,90).
