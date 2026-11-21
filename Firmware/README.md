# Firmware — bản chốt RT44-TURN / NET44

Chỉ còn **một bản** trong repo. Các bản cũ (RT29 → RT43) đã xoá khỏi cây làm việc cho gọn; nếu cần
lấy lại thì còn nguyên trong **git history** (`git log --oneline -- Firmware`).

| Board | Build ID | Tệp nạp |
|---|---|---|
| STM32F411CEU6 | `BAL-20261120-R12-8DIR-RT44-TURN` | `STM32_BAL_R12_RT44_TURN.hex` (khuyến nghị) · `.bin` (nạp thô tại `0x08000000`) · `.elf` (chỉ để debug) |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT44-TURN-NET44` | `ESP32_RT44_TURN_NET44.app.bin` (`0x10000`) + `bootloader.bin` (`0x0`) + `partitions.bin` (`0x8000`) + `boot_app0.bin` (`0xe000`) |

Ba mức tốc độ **50 / 80 / MAX 90 cm/s**; cua 0,719 m/s (tự hạ khi bánh sát trần duty); bù lệch bánh
trái 110 PWM.

> **Nạp cả hai board trong cùng một lượt.** UART giữa hai board chạy 921600 từ RT41; gateway đối chiếu
> build ID với STM32 (`$BINFO`), lệch bản là trang khoá nút BẬT CÂN BẰNG.

## 1. Nạp bằng IDE (cách bạn đang dùng)

| Board | Mở ở đâu |
|---|---|
| STM32 | STM32CubeIDE → workspace **`D:\CubeIDE_ws`** → project `Xe_Can_Bang` (source `D:\DA_HTN\Xe_Can_Bang`) → **Build** → **Run** |
| ESP32-S3 | Arduino IDE → **`D:\DA_HTN\ESP32_S3_Gateway\ESP32_S3_Gateway.ino`** → **Upload** |

Tools cho Arduino IDE: Board **ESP32S3 Dev Module**, USB CDC On Boot **Enabled**, Flash Size
**16MB (128Mb)**, Partition **16M Flash (3MB APP/9.9MB FATFS)**, PSRAM **OPI PSRAM**, Upload **921600**.

Hướng dẫn chi tiết từng bước: [`../Docs/Nap_Bang_IDE.md`](../Docs/Nap_Bang_IDE.md).

## 2. Nạp bằng script

Nháy đúp `nap_rt44.bat` trong thư mục này → nhập **số COM của ESP32** (bắt buộc, vì phải nạp cả hai
board). Hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT44_TURN_20261120'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT44_TURN.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT44_TURN_NET44.bootloader.bin" `
  0x8000 "$rel\ESP32_RT44_TURN_NET44.partitions.bin" `
  0xe000 "$rel\ESP32_RT44_TURN_NET44.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT44_TURN_NET44.app.bin"
```

## 3. Kiểm sau khi nạp

1. Cấp nguồn, chờ ~5 s (STM32 hiệu chuẩn gyro).
2. Wi-Fi `TWIP_RACER_S3` / `12345678` → `http://192.168.4.1/ui/r12`.
3. Banner phải là **`RT44-TURN-NET44`**, ba nút **50 / 80 / MAX 90**.
4. Serial Monitor ESP32 (115200): có `[UART] … @ 921600 bps` và `[NET41] … -> AP chN`.
5. Tải một log: dòng `$BINFO` có `BAL-20261120-R12-8DIR-RT44-TURN`, có `# RADIO ch=… rssi=… phy=…`,
   và `crc_errors=0`.

## 4. Hash

`SHA256.json` (7 tệp binary) và `SOURCE_SHA256.json` (16 tệp nguồn đã sinh ra binary) trong thư mục
này. Kiểm nhanh:

```powershell
cd D:\DA_HTN\Firmware\RT44_TURN_20261120
Get-Content SHA256.json | ConvertFrom-Json | ForEach-Object {
  $_.PSObject.Properties | ForEach-Object {
    $f=$_.Name; $h=(Get-FileHash $f -Algorithm SHA256).Hash.ToLower()
    "{0,-40} {1}" -f $f, ($(if($h -eq $_.Value){'OK'}else{'SAI'}))
  }
}
```

## 5. Lưu ý vận hành

- Mức **MAX 90** chỉ đạt đúng 0,90 m/s khi pin **≥ 12,0 V**; pack yếu thì trần duty tự hạ về ~0,80
  (RT43) để không bão hoà bánh và không ngã.
- Bánh trái cần ~11 % duty nhiều hơn bánh phải (cơ khí). Nếu chạy thẳng mà `v_l − v_r` > 0,15 m/s,
  cần kiểm bạc đạn/belt/motor trái — không chỉnh gain.
- Sau mỗi lượt: **STOP → chờ ~2 s → tải log**.
- Không nạp khi xe đang cân bằng hoặc đang chạy; bánh kê cao khi nạp.
