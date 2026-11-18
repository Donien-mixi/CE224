# Bản chốt RT38 — chuyển hướng liên tục, hết khựng

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261118-R12-8DIR-RT38-CONT` |
| ESP32-S3 | `TWIP-WEB-20261118-BAL-R12-8DIR-RT38-CONT-NET38` |

## 1. Sửa gì

Người vận hành báo RT37: 90 cm/s không ổn định, trôi và khựng; 50/80 đổi hướng đột ngột thì xe phải
khựng giữ cân bằng thay vì chạy liên tục.

| # | Nguyên nhân (đo trên log RT36 `(2)(5)`) | Sửa |
|---|---|---|
| 1 | Đổi nút tạo vài gói `neutral` → phanh ngay; sau đảo chiều **72 % mẫu bị phanh** | Giữ lệnh cũ tối đa **120 ms**; lệnh mới trong cửa sổ đó là đảo chiều liên tục |
| 2 | Mốc vượt 20 cm → `target=0`, tốc độ tụt rồi ramp lại (khựng ở 90) | Neo mốc tại xe +20 cm và **giữ tốc độ lệnh** khi đang chạy; xe đứng vẫn cắt để bắt tường |
| 3 | REC1 vào **ngay** khi gyro ≥150 °/s kể cả lúc đang giữ lệnh; đảo chiều tạo spike 150–250 °/s → **52 % phase 2** trong 1,2 s | Khi có lệnh, gyro phải duy trì **60 ms**; không có lệnh vẫn vào ngay |

Không đổi gain/observer/phân bổ R12, trần cua 0,749, neo mốc khi đứng yên, phanh 0,85, trim 110 PWM,
khung 122 B, lease 350 ms.

Chi tiết: [`../../Docs/RT38_Chuyen_Huong_Lien_Tuc.md`](../../Docs/RT38_Chuyen_Huong_Lien_Tuc.md).

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **39/39 pass**; `test_rt38_continuous`, `test_rt38_recovery` — mỗi test có probe hằng số cũ phải fail |
| Mô phỏng `--stress` @MAX 80 | 12/12 |
| Mô phỏng `--stress` @MAX 90 | 11/12 (ca còn lại là pin 11,2 V, như các bản trước) |
| `--stress --edge` / `--launch50` | 12/12 và 12/12 |
| Build | STM32 84 060 B bin, 0 lỗi 0 cảnh báo; ESP32 1 090 907 B |
| Hash | `SHA256.json` (7 tệp) + `SOURCE_SHA256.json` (13 tệp nguồn) |

## 3. Nạp

`nap_rt38.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT38_CONT_20261118'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT38_CONT.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT38_CONT_NET38.bootloader.bin" `
  0x8000 "$rel\ESP32_RT38_CONT_NET38.partitions.bin" `
  0xe000 "$rel\ESP32_RT38_CONT_NET38.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT38_CONT_NET38.app.bin"
```

Kiểm: banner **`RT38-CONT-NET38`**, ba nút **50 / 80 / MAX 90**.

## 4. Giới hạn

- **Chưa kiểm trên xe.** Số đo trong tài liệu RT38 là của bản RT36; RT38 mới qua host test + mô phỏng.
- Cần log theo 4 tình huống ở mục 5 của tài liệu RT38 để xác nhận.
- Mức 90 vẫn cần pin đo **≥ 12,0 V**; cua vẫn hạ về ~0,75 m/s.
