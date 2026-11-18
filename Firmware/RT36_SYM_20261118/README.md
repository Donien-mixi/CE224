# Bản chốt RT36 — sửa 4 lỗi đo từ log 18/11/2026

> **RT37 (18/11) kế tiếp:** nâng ngưỡng nghiêng governor 9°/11° → 10°/13° để hết ngập ngừng khi
> xuất phát/tăng tốc. Bản đang dùng: [`../RT37_SMOOTH_20261118/`](../RT37_SMOOTH_20261118/README.md).

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261118-R12-8DIR-RT36-SYM` |
| ESP32-S3 | `TWIP-WEB-20261118-BAL-R12-8DIR-RT36-SYM-NET36` |

## 1. Sửa gì

| # | Lỗi người vận hành báo | Nguyên nhân đo được | Sửa |
|---|---|---|---|
| 1 | Đứng yên còn dao động quanh vị trí ban đầu | mốc `x_ref` cách xe **7 cm**; PWM +300 giữ liên tục, trượt 3–11 cm (stick-slip) | sau **3 s** đứng yên thật, mốc chảy về vị trí đo **2 cm/s**, phạm vi 15 cm |
| 2 | Tiến chậm hơn lùi, hiếm khi đạt 90 | governor kẹp mốc ở **+10 cm** khi áp lực do lag 0,108; duty chỉ 0,83 | neo mốc chỉ khi **duty > 0,95** (bão hoà thật) |
| 3 | Cua trái chậm hơn cua phải | cùng kẹp mốc; cua trái `v` 0,61/0,80 trong khi cua phải 0,68–0,82 | như trên + trần cua **0,90 − \|w\|·0,131 = 0,749** |
| 4 | Cua phải-lùi ngập ngừng, nhả lệnh còn trôi | trần ngang tính theo `\|v\|` đo nên `w_ref` nhảy 1,09–1,15 (512 = 87 %); ramp phanh bị kẹp 0,65 m/s² | trần ngang theo tốc độ **đã lệnh**; cap phanh **0,85 m/s²** |

Không đổi gain/observer/phân bổ lực R12, `CAT_VNL`, khung 122 B, lease, REC1, interlock, trim 110 PWM.
Chi tiết số đo: [`../../Docs/RT36_Sua_4_Loi.md`](../../Docs/RT36_Sua_4_Loi.md).

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **36/36 pass**; 4 test mới `test_rt36_anchor`, `test_rt36_lateral`, `test_rt36_hold`, `test_rt36_brake` — mỗi test có old-constant probe phải fail |
| Mô phỏng `--stress` @MAX 80 | 12/12 |
| Mô phỏng `--stress` @MAX 90 | 11/12 — ca còn lại là pin 11,2 V (vùng bị cấm), như RT35 |
| `--stress --edge` / `--launch50` | 12/12 và 12/12 |
| Build | STM32 83 152 B text, 0 lỗi 0 cảnh báo; ESP32 1 090 907 B |
| Hash | `SHA256.json` (7 tệp) + `SOURCE_SHA256.json` (13 tệp nguồn) |

## 3. Nạp

`nap_rt36.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT36_SYM_20261118'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT36_SYM.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT36_SYM_NET36.bootloader.bin" `
  0x8000 "$rel\ESP32_RT36_SYM_NET36.partitions.bin" `
  0xe000 "$rel\ESP32_RT36_SYM_NET36.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT36_SYM_NET36.app.bin"
```

Kiểm: banner **`RT36-SYM-NET36`**, ba nút **50 / 80 / MAX 90**.

## 4. Điều kiện vận hành

- MAX 90 chỉ khi pin đo **≥ 12,0 V**; 11,7–12,0 V dùng 50/80.
- Cua (cả hai chiều) giữ **~0,75 m/s** — cố ý, không phải lỗi.
- Nhả lệnh từ 0,90 phanh trong ~1,3 s / ~0,63 m (trước 1,5–1,8 s / ~1 m).
- Sau mỗi lượt: STOP → chờ 2 s → tải log.

## 5. Giới hạn trung thực

- **Chưa kiểm trên xe.** Toàn bộ số đo trong tài liệu RT36 là của bản **RT34**; RT36 mới qua host
  test và mô phỏng. Cần một ma trận log (đứng yên / 90 tiến-lùi / cua 4 tổ hợp / nhả lệnh) để xác nhận.
- Mô phỏng chưa có trượt bánh và tụt áp theo tải, nên không thay thế log xe.
- Trần cua 0,749 làm cua chậm hơn RT34 (0,80) — đúng yêu cầu 70–75 cm/s, đổi lại quãng cua xa hơn.
