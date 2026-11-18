# Bản chốt RT34 — cua mượt ở MAX 90 (trần tốc độ do đường) · 17/11/2026

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261117-R12-8DIR-RT34-CORNER` |
| ESP32-S3 | `TWIP-WEB-20261117-BAL-R12-8DIR-RT34-CORNER-NET35` |

## 1. Sửa gì và vì sao

Log 38 (RT32, MAX 90, rẽ khi đang chạy 0,90 m/s) cho thấy cú cua bị **giật**:

| Hiện tượng đo được | Số liệu |
|---|---|
| Yaw vọt trên tham chiếu khi vào cua | **+37 %** (tham chiếu 1,15 → đo 1,58 rad/s) rồi hồi dần ~2 s |
| Bánh ngoài bão hoà | PWM **2499 (100 %)** nhiều mẫu |
| Tốc độ xe tự tụt trong cua | **0,90 → 0,66 m/s** (tham chiếu vẫn 0,90) |
| Cờ governor (16384) trong cua | **50–100 %** thời gian |

Nguyên nhân vật lý: khi cua, bánh ngoài phải chạy `v + |w|·0,131`. Ở 0,90 m/s với cua 1,15 rad/s
thì cần **1,05 m/s**, trong khi động cơ chỉ tới ~0,95 m/s ở pack 11,7 V ⇒ bánh bão hoà, vòng yaw
(R12, đóng băng) vọt rồi mới kéo lại — đó chính là cảm giác “không mượt”.

**RT34 hạ tham chiếu tốc độ khi đang cua** xuống mức bánh còn làm được:

```
giới hạn = 0,95 − |w_ref| · 0,131      (chỉ HẠ, không bao giờ nâng)
```

- Đi qua ramp của `BalanceMotion` (`BALANCE_MOTION_DECEL 0,90 m/s²`) nên tốc độ đổi **mượt**, không
  bước nhảy; `x_ref` vẫn tích phân đúng theo `v_ref` nên không sinh sai lệch vị trí.
- Chỉ áp dụng khi **đang cua** (`|w_ref| > 0,30`) và **chỉ khi tham chiếu đang cao hơn** giới hạn:
  - MAX 90 + cua 1,15 rad/s → 0,90 → **0,80 m/s** (đúng mức bánh còn dư).
  - MAX 80 + cua → **không đổi** (0,80 < 0,80… giới hạn 0,80).
  - Chạy thẳng 0,90 → **không đổi** (giới hạn 0,95).
- Không đổi gain/observer/phân bổ lực R12, khung 122 B, lease, STOP, REC1, interlock, bù bánh trái.

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **32/32 pass**, có `test_rt34_corner` (thẳng 0,90 giữ nguyên; vào cua hạ về 0,803 **không tick nào nhảy quá 0,012**; nhả cua về lại 0,90; preset 80 không đổi; **probe tắt giới hạn phải fail**) |
| Mô phỏng `--stress` @MAX 80 | 12/12 |
| Mô phỏng `--stress` @MAX 90 | **11/12** sau khi fit lại plant theo log xe ([RT35](../../Docs/RT35_Mo_Phong_Khop_Log.md)); ca còn lại là **pin 11,2 V** — vùng bị cấm với mức 90 |
| `--stress --edge` / `--launch50` | 12/12 và 12/12 |
| Build | STM32 0 lỗi 0 cảnh báo; ESP32 1 090 923 B |

Ngoài ra đã sửa **một lỗi trong test cũ**: `test_rt24_sym.cjs` khai báo `samples[117]` nhưng ghi tới
281 mẫu (tràn mảng, chỉ lộ ra khi bố cục bộ nhớ đổi) — nay là `samples[600]` có kiểm biên, và test
vẫn khẳng định đúng hành vi cũ.

## 3. Nạp

`D:\DA_HTN\Firmware\RT34_CORNER_20261117\nap_rt34.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT34_CORNER_20261117'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT34_CORNER.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT34_CORNER_NET35.bootloader.bin" `
  0x8000 "$rel\ESP32_RT34_CORNER_NET35.partitions.bin" `
  0xe000 "$rel\ESP32_RT34_CORNER_NET35.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT34_CORNER_NET35.app.bin"
```

Kiểm: banner **`RT34-CORNER-NET35`**, ba nút **50 / 80 / MAX 90**.

## 4. Điều kiện vận hành (không đổi)

- MAX 90 chỉ khi pin đo **≥ 12,0 V**; 11,7–12,0 V dùng 50/80.
- Cua ở MAX 90 giờ **tự giảm tốc về ~0,80 m/s** — đó là hành vi mong muốn, không phải lỗi.
- Sau mỗi lượt: STOP → chờ 2 s → tải log (NET35 chỉ ghi Flash khi đã dừng).

## 5. Giới hạn trung thực

- Bản này làm cua **êm hơn** bằng cách giảm tốc trong cua; muốn **vừa 0,90 vừa cua gắt** là bất khả
  thi trên 3S (bánh ngoài cần 1,05 m/s > trần động cơ). Muốn vậy phải 4S.
- Bù bánh trái (110 PWM) vẫn giữ; nó cân mô-men, không làm bánh trái khoẻ thêm.
- **Đã kiểm trên xe**: log RT34 173 s ở pack 11,7 V — 0 ngã, 0 tự ngắt, bốn hướng lái lệch 1–2 %,
  cờ 512 trong cua còn 19–26 % (trước 50–77 %). Còn lại: cua trái vẫn ngả nhiều hơn cua phải
  (~5° so với ~1,4°) — phần cơ khí, không phải luật điều khiển.
