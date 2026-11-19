# Năm bản firmware — chọn bản nào và nạp thế nào

Năm bản nằm sẵn trong năm thư mục con, **mỗi bản có đủ file cho cả STM32 và ESP32-S3**. Chỉ nạp khi
hai board cùng một bản: gateway đọc build ID từ STM32 (`$BINFO`), lệch bản là trang web khoá điều
khiển.

## 1. Chọn bản nào?

| # | Bản | Build ID (STM32 / ESP32) | Tốc độ | Có gì | Khi nào chọn | Đã kiểm trên xe |
|---|---|---|---|---|---|---|
| **1** | **RT39-CORNER / NET39** (hiện hành) | `BAL-20261119-R12-8DIR-RT39-CORNER` / `TWIP-WEB-20261119-…-RT39-CORNER-NET39` | 50 / 80 / **90** cm/s | RT38 + **một guard vượt tốc độ theo trần interlock** (hết tự vào REC1 khi cua/nhả lệnh ở 80) + **trần cua 0,87** (cua 72 cm/s, bánh ngoài còn ~10 % duty) | Mặc định. Bản sửa lỗi “80 cm/s khựng và dừng lâu khi cua” | **Chưa** — 40/40 host test + mô phỏng |
| **2** | **RT38-CONT / NET38** | `BAL-20261118-…-RT38-CONT` / `TWIP-WEB-20261118-…-RT38-CONT-NET38` | 50 / 80 / **90** cm/s | RT37 + đổi hướng liên tục (giữ lệnh 120 ms), neo mốc thay vì cắt tốc độ khi lệch 20 cm, REC1 do gyro cần 60 ms khi có lệnh | Bản đã có log xe `(39)` — nhưng log đó chính là ca khựng ở 80 | **Có** — log `(39)`, 84 s, 0 ngã |
| **3** | **RT37-SMOOTH / NET37** | `BAL-20261118-…-RT37-SMOOTH` / `TWIP-WEB-20261118-…-NET37` | 50 / 80 / **90** cm/s | RT36 + ngưỡng nghiêng governor 10°/13° | Bản thử ngay trước RT38 | **Chưa** |
| **4** | **RT34-CORNER / NET35** | `BAL-20261117-…-RT34-CORNER` / `TWIP-WEB-20261117-…-NET35` | 50 / 80 / **90** cm/s | Ba mức + bù bánh trái 110 PWM + trần cua 0,80 | Khi RT39/RT38 có hành vi lạ; đây là bản **đã có log 173 s, 0 ngã** | **Có** — log 17/11 |
| **5** | **RT29-NOCUT / NET30** (dự phòng) | `BAL-20261115-…-RT29-NOCUT` / `TWIP-WEB-20261115-…-NET30` | 50 / **72 / 80** cm/s | Bản cũ, **không** có bù bánh trái, **không** có trần cua, **không** có mức 90 | Khi cần quay về bản đơn giản nhất từng chạy ổn; hoặc để so sánh | **Có** — nhiều lượt trước 15/11 |

Bảng chọn theo triệu chứng:

| Hiện tượng | Nên thử |
|---|---|
| RT39 vẫn khựng khi cua ở 80 | Gửi log mới; kiểm `control_phase=2` có còn > 5 % không, và duty bánh trái có ghim 1,00 không |
| RT39 cua chậm hơn mong muốn | Đây là chủ ý (72 thay vì 75 cm/s); muốn nhanh hơn phải chấp nhận bánh ngoài sát trần |
| RT38 khựng khi đổi hướng, dừng lâu khi cua | Nạp **RT39** |
| Cần bản “chắc” nhất để demo, không cần 90 | **RT34** (đã có log 173 s) hoặc **RT29** |
| Trang báo “Chờ thông tin phiên bản STM32…” | Hai board lệch bản — nạp lại **cả hai** cùng một bản |
| Muốn thử 90 cm/s | RT39/RT38/RT37/RT34, **chỉ khi pin đo ≥ 12,0 V** |

## 2. File trong mỗi thư mục

| Tệp | Nạp vào |
|---|---|
| `STM32_BAL_R12_*.hex` | STM32 qua ST-LINK (khuyến nghị) |
| `STM32_BAL_R12_*.bin` | STM32 nạp thô tại `0x08000000` |
| `STM32_BAL_R12_*.elf` | Ký hiệu debug, không nạp |
| `ESP32_*.bootloader.bin` | ESP32 tại `0x0` |
| `ESP32_*.partitions.bin` | ESP32 tại `0x8000` |
| `ESP32_*.boot_app0.bin` | ESP32 tại `0xe000` |
| `ESP32_*.app.bin` | ESP32 tại `0x10000` (ứng dụng + web) |
| `SHA256.json` | Hash 7 tệp trên — kiểm trước khi nạp |
| `SOURCE_SHA256.json` | Hash các tệp nguồn đã sinh ra bản đó (chỉ RT39 khớp source hiện tại) |
| `nap_*.bat` | Nạp tự động cả hai board |

## 3. Chuẩn bị

1. Cáp **USB dữ liệu** cho ESP32 (nhiều cáp chỉ sạc), cáp ST-LINK cho STM32.
2. Đóng Arduino IDE, STM32CubeProgrammer, mọi phần mềm đang giữ COM.
3. Xe đứng yên, **bánh kê cao hoặc tắt động cơ** trong lúc nạp; pin đủ (đừng nạp khi pack cạn).
4. Cắm ST-LINK và ESP32 vào máy; mở Device Manager xem **COM của ESP32** (CP210x/CH340).
5. Kiểm hash (tuỳ chọn nhưng nên làm):

```powershell
cd D:\DA_HTN\Firmware\RT39_CORNER_20261119
Get-Content SHA256.json | ConvertFrom-Json | ForEach-Object {
  $_.PSObject.Properties | ForEach-Object {
    $f=$_.Name; $h=(Get-FileHash $f -Algorithm SHA256).Hash.ToLower()
    "{0,-42} {1}" -f $f, ($(if($h -eq $_.Value){'OK'}else{'SAI'}))
  }
}
```

## 4. Cách nạp

### Cách A — một script cho mỗi bản (dễ nhất)

Nháy đúp script trong thư mục bản muốn nạp:

| Bản | Script |
|---|---|
| RT39-CORNER | `RT39_CORNER_20261119\nap_rt39.bat` |
| RT38-CONT | `RT38_CONT_20261118\nap_rt38.bat` |
| RT37-SMOOTH | `RT37_SMOOTH_20261118\nap_rt37.bat` |
| RT34-CORNER | `RT34_CORNER_20261117\nap_rt34.bat` |
| RT29-NOCUT | `RT29_NOCUT_20261115\nap_rt29.bat` |

Script hỏi **số COM của ESP32** (ví dụ `COM3`). Nhập xong nó nạp STM32 trước, rồi ESP32.
Nếu **bỏ trống COM**, script chỉ nạp STM32.

### Cách B — menu chọn bản

Nháy đúp `Firmware\nap_chon_ban.bat`, chọn `1` / `2` / `3` / `4` / `5` rồi nhập COM. Menu gọi đúng
script ở cách A.

### Cách C — nạp tay

STM32 (thay tên tệp theo bản):

```powershell
& "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe" `
  -c port=SWD -w "D:\DA_HTN\Firmware\RT39_CORNER_20261119\STM32_BAL_R12_RT39_CORNER.hex" -v -rst
```

ESP32 (đổi tên tệp theo bản và COM):

```powershell
$rel='D:\DA_HTN\Firmware\RT39_CORNER_20261119'
$es='C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT39_CORNER_NET39.bootloader.bin" `
  0x8000 "$rel\ESP32_RT39_CORNER_NET39.partitions.bin" `
  0xe000 "$rel\ESP32_RT39_CORNER_NET39.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT39_CORNER_NET39.app.bin"
```

> Dùng **esptool v5**: lệnh là `write-flash` (có gạch nối). Bản v3 cũ dùng `write_flash`.

### Cách D — nạp bằng IDE (Build + Run / Upload)

Nếu muốn mở **STM32CubeIDE** bấm Build → Run và mở **Arduino IDE** bấm Upload thay vì dùng script,
xem hướng dẫn riêng: [`../Docs/Nap_Bang_IDE.md`](../Docs/Nap_Bang_IDE.md). Năm bản đều có source sẵn
để mở (RT39 dùng cây hiện tại; RT38/RT37/RT34/RT29 dùng snapshot trong `.build\ide_src`, tạo lại bằng
`tools\ide_sources.ps1`).

## 5. Kiểm sau khi nạp

1. Rút ST-LINK, cấp nguồn xe, chờ ~5 s (STM32 hiệu chuẩn gyro 500 mẫu).
2. Kết nối Wi-Fi `TWIP_RACER_S3` / `12345678`, mở `http://192.168.4.1/ui/r12`.
3. Banner phải đúng bản:

| Bản | Banner | Nút tốc độ |
|---|---|---|
| RT39 | `RT39-CORNER-NET39` | 50 / 80 / MAX 90 |
| RT38 | `RT38-CONT-NET38` | 50 / 80 / MAX 90 |
| RT37 | `RT37-SMOOTH-NET37` | 50 / 80 / MAX 90 |
| RT34 | `RT34-CORNER-NET35` | 50 / 80 / MAX 90 |
| RT29 | `RT29-NOCUT-NET30` | 50 / 72 / 80 |

4. Trong log tải về, dòng `$BINFO` phải chứa đúng build ID của bản vừa nạp.
5. Nếu trang báo “Chờ thông tin phiên bản STM32…” hoặc nút BẬT CÂN BẰNG bị khoá: hai board đang
   lệch bản → nạp lại cả hai.

## 6. Lưu ý khi đổi bản

- RT29 **không có** mức 90 và **không có** bù bánh trái/trần cua → cua và chạy thẳng sẽ khác RT34/RT37/RT38/RT39.
- RT39 **chưa có log xe**; RT38 có log `(39)` nhưng đó là ca khựng ở 80 mà RT39 sửa.
- RT39 cua ở 72 cm/s (thay vì 75) để bánh ngoài còn dự trữ duty.
- Mức 90 chỉ dùng khi **pin đo ≥ 12,0 V**; dưới ~11,9 V governor sẽ ghìm.
- Sau mỗi lượt: STOP → chờ 2 s → tải log. `archive_error=1` vẫn cần bấm “SỬA VÙNG LOG FLASH” một lần
  khi motor đã dừng (xem HANDOFF).
- Không nạp khi xe đang cân bằng hoặc đang chạy.
