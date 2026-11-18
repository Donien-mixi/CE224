# Firmware RT29-NOCUT / NET30 — bản dự phòng (15/11/2026)

**Đây là bản dự phòng, không phải bản hiện hành.** Bản chốt đang dùng là
[`../RT37_SMOOTH_20261118/`](../RT37_SMOOTH_20261118/README.md) (RT37-SMOOTH / NET37, ba mức 50/80/90).
Số liệu kiểm thử dưới đây là tại thời điểm phát hành 15/11/2026 (30 test, ba mức 50/72/80).

Cặp firmware này nạp **cả hai board** cùng bản: gateway đối chiếu
build ID nhận được từ STM32 (`$BINFO`), lệch bản/ngày thì trang web khoá điều khiển và nút
**ĐO LẠI GYRO** không chọn được.

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261115-R12-8DIR-RT29-NOCUT` |
| ESP32-S3 | `TWIP-WEB-20261115-BAL-R12-8DIR-RT29-NOCUT-NET30` |

## 1. Nội dung thư mục

| Tệp | Dùng để |
|---|---|
| `STM32_BAL_R12_RT29_NOCUT.hex` | Nạp STM32 bằng ST-LINK / STM32CubeProgrammer (khuyến nghị). |
| `STM32_BAL_R12_RT29_NOCUT.bin` | Nạp thô tại địa chỉ `0x08000000`. |
| `STM32_BAL_R12_RT29_NOCUT.elf` | Ký hiệu để debug, xem map khi cần. |
| `ESP32_RT29_NOCUT_NET30.bootloader.bin` | esptool `0x0`. |
| `ESP32_RT29_NOCUT_NET30.partitions.bin` | esptool `0x8000`. |
| `ESP32_RT29_NOCUT_NET30.boot_app0.bin` | esptool `0xe000`. |
| `ESP32_RT29_NOCUT_NET30.app.bin` | esptool `0x10000` (ứng dụng + web). |
| `SHA256.json` | Hash của các tệp trên — kiểm trước khi nạp. |
| `SOURCE_SHA256.json` | Hash của 12 tệp nguồn đã sinh ra đúng hai binary này. |
| `nap_rt29.bat` | Nạp tự động **cả hai board** (nhập COM ESP32). Xem [hướng dẫn chọn bản](../README.md). |

Bản `merged.bin` (ảnh gộp 16 MB để nạp một lần) **không** đưa vào repo; tạo lại bằng:

```powershell
esptool.py --chip esp32s3 merge_bin -o merged.bin --flash_mode dio --flash_freq 80m --flash_size 16MB `
  0x0     ESP32_RT29_NOCUT_NET30.bootloader.bin `
  0x8000  ESP32_RT29_NOCUT_NET30.partitions.bin `
  0xe000  ESP32_RT29_NOCUT_NET30.boot_app0.bin `
  0x10000 ESP32_RT29_NOCUT_NET30.app.bin
```

## 2. Kiểm hash trước khi nạp

```powershell
$dir = 'Firmware/RT29_NOCUT_20261115'
$m = Get-Content "$dir/SHA256.json" -Raw | ConvertFrom-Json
$m.PSObject.Properties | ForEach-Object {
  $h = (Get-FileHash (Join-Path $dir $_.Name)).Hash.ToLower()
  "{0,-48} {1}" -f $_.Name, $(if ($h -eq $_.Value) { 'OK' } else { 'LỆCH' })
}
```

## 3. Nạp STM32 (COM qua ST-LINK)

Cách nhanh: nháy đúp `nap_rt29.bat`, nhập COM ESP32 (bỏ trống nếu chỉ nạp STM32). Hoặc nạp tay:

```powershell
STM32_Programmer_CLI.exe -c port=SWD -w STM32_BAL_R12_RT29_NOCUT.hex -v -rst
```

Sau khi nạp, LED chậm nháy ở trạng thái STANDBY; hiệu chuẩn gyro lúc cấp nguồn mất khoảng 5,15 s
(500 mẫu) trước khi xe sẵn sàng. Bản này dùng lệnh STOP/ARM giống bản trước, không có thay đổi
giao thức.

## 4. Nạp ESP32-S3 (COM3, 16 MB flash)

```powershell
$es = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0     ESP32_RT29_NOCUT_NET30.bootloader.bin `
  0x8000  ESP32_RT29_NOCUT_NET30.partitions.bin `
  0xe000  ESP32_RT29_NOCUT_NET30.boot_app0.bin `
  0x10000 ESP32_RT29_NOCUT_NET30.app.bin
```

Nạp xong, kết nối Wi-Fi `TWIP_RACER_S3` (mật khẩu `12345678`) và mở `http://192.168.4.1/ui/r12`.
Banner phải hiện `RT29-NOCUT-NET30`; nếu hiện bản khác thì ESP32 chưa được nạp đúng tệp.

## 5. Trạng thái kiểm chứng của bản này

- **Đã kiểm trên máy:** 30/30 host test pass; 36/36 ca mô phỏng native đủ 20/20 s không fault;
  12/12 tệp nguồn khớp hash trong `SOURCE_SHA256.json`.
- **Đã kiểm trên xe:** chưa có log cho riêng RT29. Log xe gần nhất là `(12)(3)` chạy bản RT28
  (3 lần tự ngắt oan `fault 4096`); RT29 chỉ thêm điều kiện miễn hạn 4 s cho phanh do nhiễu, không
  đổi gain, observer, phân bổ lực hay khung truyền.
- Bản RT28 (bản ngay trước) đã có log xe `(12)(3)`: cân bằng vẫn giữ khi bị đẩy, nhưng xe tự ngắt
  3 lần với `fault 4096` — đó chính là lỗi RT29 sửa. Xem
  [`../../Docs/RT29_Tu_Ngat_4096.md`](../../Docs/RT29_Tu_Ngat_4096.md).

## 6. Nguồn tương ứng

Nếu cần build lại từ source thay vì dùng binary, dùng đúng commit có các hash trong
`SOURCE_SHA256.json` (12 tệp: `Core/Inc/balance_*.h`, `Core/Src/robot_fsm.c`, và 5 header gateway).
Sửa bất kỳ tệp nào trong đó thì binary không còn tương ứng và build ID phải đổi ở cả hai board
(`tools/test_build_id_sync.cjs` sẽ báo nếu lệch literal).
