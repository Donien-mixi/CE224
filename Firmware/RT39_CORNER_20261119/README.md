# Bản chốt RT39 — mốc 80 cm/s vào cua không còn tự dừng

Nạp **cả hai board** (STM32 và ESP32 đều đổi build ID).

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261119-R12-8DIR-RT39-CORNER` |
| ESP32-S3 | `TWIP-WEB-20261119-BAL-R12-8DIR-RT39-CORNER-NET39` |

## 1. Sửa gì

Người vận hành báo bản RT38: ở mốc **80 cm/s**, xe khựng khi cua/đánh lái và **có lúc dừng rất lâu để
cân bằng ở cả bốn hướng rẽ**. Không phải motor yếu và không phải lỗi encoder — log `(39)` sạch
(`crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0`, `age` p50 13 ms, không có `fault` nào khi
`state=3`).

Đo trên log `twip_balance_R12 (39).txt` (RT38, 4 198 mẫu = 84 s, preset 80, cả bốn hướng):

| Số đo | Giá trị |
|---|---|
| Thời gian ở REC1 (`control_phase=2`) | **25,3 s = 30,2 % cả lượt chạy**, 12 cước, p50 1,94 s, max 2,76 s |
| Số cước REC1 vào **trong lúc đang giữ lệnh** | 11/12 |
| Vào REC1 khi đang cua 0,77–0,79 m/s trên tham chiếu 0,749 | 4 cước (sai lệch chỉ 0,03–0,04 m/s) |
| Vào REC1 ngay sau khi nhả tham chiếu (0,769 so với 0,651) | 2 cước |
| Tổng thời gian "có lệnh mà |v| < 0,10 m/s" | 12,5 s = 18,8 % thời gian có lệnh |

| # | Nguyên nhân | Sửa |
|---|---|---|
| 1 | Guard vượt tốc độ so bánh với **tham chiếu tức thời**: `|v\| > 0,78` **và** `\|v−v_ref\| > 0,04` (nhánh thứ hai `> 0,72` và `> 0,12`). Ở mốc 80 xe chạy bình thường 0,78–0,84 m/s trong khi cua hạ tham chiếu xuống 0,749, nên **mỗi lần vào cua và mỗi lần nhả lệnh đều bị coi là vượt tốc độ** → REC1 phanh 0,78 → 0 trong 1,9–2,8 s | Một guard duy nhất, so với **trần của interlock** `max(envelope, |v_ref|+0,12)` và cần **100 ms** bằng chứng; bỏ điều kiện nghiêng `|pitch|>5 or |gyro|>60` (nó không bắt được xe chạy thẳng lăn nhanh trên thân phẳng) |
| 2 | Trần cua đặt bánh ngoài đúng **0,90 m/s** = 94–96 % duty ở pack 11,2–11,7 V, không còn dự trữ cho vòng cân bằng → duty ghim 1,00 (33 % số mẫu cua lùi-phải), thân nghiêng 6–16°, có lúc tự vào REC1 vì nghiêng | Hạ `BALANCE_TURN_VNL_BUDGET` 0,90 → **0,87**; cua còn **0,719 m/s (72 cm/s)** và bánh ngoài 0,87 m/s ≈ 88–90 % duty |

Không đổi gain/observer/phân bổ R12, neo mốc, phanh 0,85, trim 110 PWM, khung 122 B, lease 350 ms,
ngưỡng REC1 do nghiêng/gyro (12°/150 °/s khi có lệnh, 8°/150 °/s khi không), interlock 200 ms.

Chi tiết: [`../../Docs/RT39_Cua_80_Khong_Khung.md`](../../Docs/RT39_Cua_80_Khong_Khung.md).

## 2. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **40/40 pass** (thêm `test_rt39_corner`); mỗi test đổi hành vi đều có probe hằng số cũ phải fail |
| `test_rt39_corner` | 438 mẫu 50 Hz **thật** cắt trước 6 cước REC1 sai: **0 mẫu** vào REC1 (probe `SLACK=0.12` → fail) |
| `test_rt12_replay72` | 38 ảnh chụp REC1 72 cm/s: 0 mẫu vào REC1; runaway 0,95 trên tham chiếu 0,50 vẫn vào |
| Mô phỏng `--stress` @MAX 80 | 12/12, mọi ca đủ 20 s |
| Mô phỏng `--stress` @MAX 90 | 11/12 — y như RT38: ca 7 ngã ở 8,95 s, số liệu trùng khớp từng chữ số |
| `--stress --edge` / `--launch50` | 12/12 và 12/12 (launch trùng RT38 tuyệt đối) |
| Build | STM32 83 420 B bin, 0 lỗi 0 cảnh báo; ESP32 1 091 072 B |
| Hash | `SHA256.json` (7 tệp) + `SOURCE_SHA256.json` (13 tệp nguồn) |

## 3. Nạp

`nap_rt39.bat` (nhập COM ESP32) hoặc lệnh tay:

```powershell
$rel = 'D:\DA_HTN\Firmware\RT39_CORNER_20261119'
$cp  = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506\tools\bin\STM32_Programmer_CLI.exe'
$es  = 'C:\Users\DONG NHIEN\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe'
& $cp -c port=SWD -w "$rel\STM32_BAL_R12_RT39_CORNER.hex" -v -rst
& $es --chip esp32s3 --port COM3 --baud 921600 write-flash `
  0x0 "$rel\ESP32_RT39_CORNER_NET39.bootloader.bin" `
  0x8000 "$rel\ESP32_RT39_CORNER_NET39.partitions.bin" `
  0xe000 "$rel\ESP32_RT39_CORNER_NET39.boot_app0.bin" `
  0x10000 "$rel\ESP32_RT39_CORNER_NET39.app.bin"
```

Nạp bằng IDE: xem [`../../Docs/Nap_Bang_IDE.md`](../../Docs/Nap_Bang_IDE.md) — chọn sketch/project
**RT39**, không chọn RT38.

Kiểm: banner **`RT39-CORNER-NET39`**, ba nút **50 / 80 / MAX 90**.

## 4. Giới hạn

- **Chưa kiểm trên xe.** Toàn bộ số đo ở trên là của log RT38 `(39)`; RT39 mới qua host test + mô phỏng.
- Cần một log mới ở mốc 80, đủ bốn hướng rẽ, để xác nhận: kỳ vọng `control_phase=2` **< 5 %** thời gian
  (thay vì 30 %) và không cước nào dài quá 0,5 s khi đang cua.
- Cua chậm hơn 3 cm/s so với RT38 (72 thay vì 75 cm/s) — đây là chủ ý để bánh ngoài còn 10 % duty.
- Bánh **trái** vẫn cần ~11 % duty/1 m/s so với bánh phải (log `(39)`: 1,135 so với 1,012). Ở hướng
  lùi-phải bánh trái chạm 0,98 duty. Đây là **cơ khí** (bạc đạn/belt/motor trái), không sửa được bằng
  luật điều khiển; RT39 chỉ giảm tải để nó không ghim 100 %.
- Mức 90 vẫn cần pin đo **≥ 12,0 V**.
