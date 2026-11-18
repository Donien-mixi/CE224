# Bản chốt RT37 — xuất phát mượt, hết ngập ngừng

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261118-R12-8DIR-RT37-SMOOTH` |
| ESP32-S3 | `TWIP-WEB-20261118-BAL-R12-8DIR-RT37-SMOOTH-NET37` |

## 1. Sửa gì

Log RT36 ngày 18/11 (`(2)(5)`, 7 699 mẫu / 154 s) cho thấy mỗi cú xuất phát mất **2,6–5,5 s** mới đạt
90 % mốc, governor bật `limited` **25–44 %** 2 s đầu, pitch tới **7–12°** và `v_ref` chỉ 0,28–0,74 sau
2 s — ramp bị cắt thành từng đoạn (ngập ngừng).

Nguyên nhân: ngưỡng nghiêng của governor (9° chạy / 11° xuất phát) thấp hơn độ ngả bình thường của xe
khi tăng tốc. RT37 nâng thành **10° / 13°**. Gia tốc mồi, crawl, phanh, trần cua, neo mốc giữ nguyên.

Đã thử và loại (mô phỏng MAX 90): giảm gia tốc mồi 0,70 → 0,45 (**9/12**), nâng crawl 0,06 → 0,12
(8–10/12). Chỉ nâng ngưỡng nghiêng giữ **11/12**, bằng mức RT35/RT36.

Chi tiết: [`../../Docs/RT37_Xuat_Phat_Muot.md`](../../Docs/RT37_Xuat_Phat_Muot.md).

## 2. Tính realtime của log nền (RT36)

| Đại lượng | Đo được |
|---|---:|
| `dt_us` | p50 4 999, min 4 996, max 5 009 µs (jitter ±9 µs) |
| `execution_us` | p50 642, p95 648, max 670 µs (13 % của 5 ms) |
| `drops` / `fault 64` | 0 / 0 |
| Tuổi lệnh | p50 13 ms, p95 27 ms, không mẫu > 350 ms |

Ngập ngừng **không** do nhịp vòng hay mất lệnh; là chính sách giới hạn gia tốc.

## 3. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **37/37 pass**; `test_rt37_smooth` có probe ngưỡng cũ 9/11 phải fail |
| Mô phỏng `--stress` @MAX 80 | 12/12 |
| Mô phỏng `--stress` @MAX 90 | 11/12 (ca còn lại là pin 11,2 V, như RT35/RT36) |
| `--stress --edge` / `--launch50` | 12/12 và 12/12 |
| Build | STM32 83 160 B text, 0 lỗi 0 cảnh báo; ESP32 1 090 907 B |
| Hash | `SHA256.json` (7 tệp) + `SOURCE_SHA256.json` (13 tệp nguồn) |

## 4. Nạp

`nap_rt37.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT37_SMOOTH_20261118'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT37_SMOOTH.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT37_SMOOTH_NET37.bootloader.bin" `
  0x8000 "$rel\ESP32_RT37_SMOOTH_NET37.partitions.bin" `
  0xe000 "$rel\ESP32_RT37_SMOOTH_NET37.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT37_SMOOTH_NET37.app.bin"
```

Kiểm: banner **`RT37-SMOOTH-NET37`**, ba nút **50 / 80 / MAX 90**.

## 5. Giới hạn

- **Chưa kiểm trên xe.** Số ở mục 1 là của RT36; RT37 mới qua host test + mô phỏng.
- Khi đạt 0,90 vẫn cần pin đo **≥ 12,0 V**; cua vẫn hạ về ~0,75 m/s (RT36).
- Không sửa gain/observer/phân bổ R12, khung 122 B, REC1, interlock, trim 110 PWM.
