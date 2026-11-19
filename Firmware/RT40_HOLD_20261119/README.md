# Bản chốt RT40 — hết ngập ngừng khi phanh, không cắt motor khi bị giữ

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261119-R12-8DIR-RT40-HOLD` |
| ESP32-S3 | `TWIP-WEB-20261119-BAL-R12-8DIR-RT40-HOLD-NET40` |

## 1. Sửa gì

Người vận hành gửi **8 log** của RT39 (cả 3 mốc, 8 hướng, chống chịu) và báo: đẩy xe thì **motor bị
ngắt**; xe **ngập ngừng, chưa thực thi ngay** khi nhận lệnh; lùi thì nhả lệnh rồi vẫn chạy thêm; rẽ
liên tục 4 hướng chưa mượt và **đã ngã**. Cả 8 log đều sạch (`crc 0`, `gaps 0`, `drops 0`, `age`
p95 28 ms) nên lỗi nằm trong luật giám sát.

| # | Nguyên nhân (đo trên 8 log) | Sửa |
|---|---|---|
| 1 | **15/28 cước REC1** vào khi đang giữ lệnh vì `\|pitch\| ≥ 12°` — nhưng đó chỉ là độ ngả **12,1–15,1°** của một cú phanh (gyro 1–61 °/s) → `v_ref` bị ghim 0 tới **2,2 s**: tổng **8,4 s “có lệnh mà không chạy”** | Ngưỡng nghiêng khi có lệnh **12° → 16°**. Giữ 8° khi không lệnh, gyro 150 °/s, sai bám 0,35 m/s, cắt 25° |
| 2 | `02_chong_chiu` bị **fault 4096 đúng 4 s** sau khi vào REC1, lúc xe **vẫn đứng** (pitch 7,6°, gyro 15 °/s) — người thử chỉ giữ xe lâu hơn hạn | Hạn REC1 **4 s → 8 s**; vẫn bị chặn bởi 50° / 1,0 m / interlock |
| 3 | **3 lần ngã** ở mốc 90 (lùi): trước cả ba, **một bánh ghim duty 100 %** khi đang cua; nhả lệnh → phanh yaw kéo bánh kia, trục vọt −0,72 → −0,95 m/s → thân quật −56° | **Trần cua theo duty**: đang cua mà có bánh ≥96 % duty thì hạ tham chiếu (sàn 0,72), hồi khi ≤90 % |

Không đổi gain/observer/phân bổ R12, trim 110 PWM, khung 122 B, lease 350 ms, phanh 0,85, neo mốc,
interlock 200 ms, mức tốc độ 50/80/90.

Chi tiết + bảng số: [`../../Docs/RT40_Het_Ngap_Ngung.md`](../../Docs/RT40_Het_Ngap_Ngung.md).

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **41/41 pass**; `test_rt40_hold` phát lại **676 nhịp 200 Hz** của 13 cửa sổ nghiêng-phanh thật → **0** cước REC1; 3 old-constant probe đều fail |
| Giữ xe 6 s | REC1 vẫn active, **không fault** (probe `TIMEOUT_MS=4000` fail) |
| Trần cua động | duty 0,99 → tham chiếu cua 0,518 m/s; thả ra hồi 1,0; chạy thẳng không bị ảnh hưởng |
| Mô phỏng @MAX 80 / edge / launch | **12/12**, **12/12**, **12/12** (pitch trùng RT39 từng ca) |
| Mô phỏng @MAX 90 | 11/12 — ca 7 ngã ở 8,87 s (RT39: 8,95 s), pin 11,2 V |
| Build | STM32 83 824 B bin, 0 lỗi 0 cảnh báo; ESP32 1 091 072 B; hex build lại trong IDE **trùng hash** |
| Hash | `SHA256.json` (7 tệp) + `SOURCE_SHA256.json` (13 tệp nguồn) |

## 3. Nạp

`nap_rt40.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT40_HOLD_20261119'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT40_HOLD.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT40_HOLD_NET40.bootloader.bin" `
  0x8000 "$rel\ESP32_RT40_HOLD_NET40.partitions.bin" `
  0xe000 "$rel\ESP32_RT40_HOLD_NET40.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT40_HOLD_NET40.app.bin"
```

Nạp bằng IDE: workspace **`D:\CubeIDE_ws`** (RT40) → Build → Run; xem
[`../../Docs/Nap_Bang_IDE.md`](../../Docs/Nap_Bang_IDE.md).

Kiểm: banner **`RT40-HOLD-NET40`**, ba nút **50 / 80 / MAX 90**.

## 4. Kịch bản đo cho log kế tiếp

1. Đứng yên 30 s → đẩy nhẹ 1 lần (không giữ) → **phải về chỗ cũ**.
2. Đẩy và **giữ 6–8 s** → **không cắt motor**, không ngã; thả → về chỗ cũ.
3. Mỗi mốc 50/80: tiến 3 s → nhả → lùi 3 s → nhả.
4. Đổi hướng liên tục (tiến 0,8 → lùi ngay) ở 50/80.
5. Rẽ 4 hướng ở 80 và 90, mỗi hướng ≥ 2 s rồi nhả.

Đạt khi: REC1 do nghiêng-khi-có-lệnh **0 cước**; không cắt motor khi bị giữ; **0 lần ngã**; duty p95
khi cua **< 0,95**; `v_ref` không ghim 0 quá 0,3 s khi đang giữ lệnh.

## 5. Giới hạn

- **Chưa kiểm trên xe**; toàn bộ số đo là của 8 log RT39.
- Mốc **90** vẫn là giới hạn của pack 3S (log 05: đặt 0,90, đạt p50 0,79) — cần pin ≥ 12,0 V.
- Bánh trái cần ~11 % duty nhiều hơn bánh phải ở mọi hướng: **cơ khí** (bạc đạn/belt/motor), không sửa
  được bằng gain; RT40 chỉ giữ dư duty để bánh không ghim 100 %.
